#!/usr/bin/env python3
"""
SPSA Tuner for Chess Engine
============================
Simultaneous Perturbation Stochastic Approximation tuner for chess engine parameters.

This script runs self-play games with perturbed parameters to find optimal values.

Usage:
    python spsa_tuner.py --engine ./engine.exe --iterations 100

For GitHub Actions, use:
    python spsa_tuner.py --engine ./output/main.exe --games-per-iter 50 --iterations 50
"""

import subprocess
import random
import os
import sys
import json
import math
import argparse
import tempfile
from dataclasses import dataclass, field
from typing import List, Dict, Tuple, Optional
from concurrent.futures import ProcessPoolExecutor, as_completed
import time

# ============================================================================
# SPSA Parameters to Tune
# ============================================================================

@dataclass
class TunableParam:
    """Represents a single tunable parameter."""
    name: str           # UCI option name
    value: float        # Current value
    min_val: float      # Minimum allowed value
    max_val: float      # Maximum allowed value
    c_end: float = 0.0  # SPSA c parameter (perturbation size) - auto-calculated if 0
    a_end: float = 0.0  # SPSA a parameter (step size) - auto-calculated if 0

    def __post_init__(self):
        # Auto-calculate c and a if not specified
        if self.c_end == 0:
            # c should be about 1-2% of the parameter range
            self.c_end = max(1, (self.max_val - self.min_val) * 0.02)
        if self.a_end == 0:
            # a should be about 10% of c
            self.a_end = self.c_end * 0.1

    def clamp(self, val: float) -> float:
        """Clamp value to valid range."""
        return max(self.min_val, min(self.max_val, val))


# Default parameters to tune (matching tuning.cpp in the engine)
DEFAULT_PARAMS = [
    # Material Values (Middlegame)
    TunableParam("PawnValueMG", 100, 70, 130),
    TunableParam("KnightValueMG", 320, 280, 360),
    TunableParam("BishopValueMG", 330, 290, 370),
    TunableParam("RookValueMG", 500, 450, 550),
    TunableParam("QueenValueMG", 950, 850, 1050),

    # Material Values (Endgame)
    TunableParam("PawnValueEG", 130, 100, 160),
    TunableParam("KnightValueEG", 340, 300, 380),
    TunableParam("BishopValueEG", 350, 310, 390),
    TunableParam("RookValueEG", 550, 500, 600),
    TunableParam("QueenValueEG", 1000, 900, 1100),

    # Piece Activity Bonuses (Middlegame)
    TunableParam("BishopPairBonusMG", 30, 0, 60),
    TunableParam("RookOpenFileBonusMG", 25, 0, 50),
    TunableParam("RookSemiOpenFileBonusMG", 11, 0, 30),
    TunableParam("RookOnSeventhBonusMG", 20, 0, 50),
    TunableParam("KnightOutpostBonusMG", 25, 0, 50),

    # Piece Activity Bonuses (Endgame)
    TunableParam("BishopPairBonusEG", 50, 20, 80),
    TunableParam("RookOpenFileBonusEG", 15, 0, 40),
    TunableParam("RookSemiOpenFileBonusEG", 3, 0, 20),
    TunableParam("RookOnSeventhBonusEG", 30, 0, 60),
    TunableParam("KnightOutpostBonusEG", 15, 0, 40),

    # Pawn Structure (Middlegame)
    TunableParam("IsolatedPawnPenaltyMG", -45, -70, -10),
    TunableParam("DoubledPawnPenaltyMG", -16, -40, 0),
    TunableParam("BackwardPawnPenaltyMG", -10, -30, 0),
    TunableParam("ConnectedPawnBonusMG", 5, 0, 20),
    TunableParam("PhalanxBonusMG", 10, 0, 25),

    # Pawn Structure (Endgame)
    TunableParam("IsolatedPawnPenaltyEG", -20, -50, 0),
    TunableParam("DoubledPawnPenaltyEG", -21, -50, 0),
    TunableParam("BackwardPawnPenaltyEG", -15, -35, 0),
    TunableParam("ConnectedPawnBonusEG", 2, 0, 15),
    TunableParam("PhalanxBonusEG", 8, 0, 20),

    # King Safety
    TunableParam("KingSafetyWeight", 83, 50, 150),
]


# ============================================================================
# SPSA Algorithm Implementation
# ============================================================================

class SPSATuner:
    """SPSA tuner for chess engine parameters."""

    def __init__(self,
                 engine_path: str,
                 params: List[TunableParam],
                 games_per_iter: int = 100,
                 time_control: str = "1+0.1",
                 concurrency: int = 1,
                 cutechess_path: str = "cutechess-cli",
                 fen_count: int = 500,
                 positions_per_iter: int = 20,
                 search_depth: int = 4):
        """
        Initialize SPSA tuner.

        Args:
            engine_path: Path to engine executable
            params: List of parameters to tune
            games_per_iter: Number of games per iteration (must be even)
            time_control: Time control string (e.g., "1+0.1" = 1s + 0.1s increment)
            concurrency: Number of concurrent games
            cutechess_path: Path to cutechess-cli
        """
        self.engine_path = os.path.abspath(engine_path)
        self.params = params
        self.games_per_iter = games_per_iter if games_per_iter % 2 == 0 else games_per_iter + 1
        self.time_control = time_control
        self.concurrency = concurrency
        self.cutechess_path = cutechess_path
        self.fen_count = fen_count
        self.positions_per_iter = positions_per_iter
        self.search_depth = search_depth

        # SPSA hyperparameters
        self.A = 10              # Stability constant (iterations)
        self.alpha = 0.602       # Step size decay exponent
        self.gamma = 0.101       # Perturbation decay exponent

        # Results tracking
        self.iteration = 0
        self.current_params = {p.name: p.value for p in params}  # Current/final params
        self.best_params = {p.name: p.value for p in params}     # Best params seen
        self.best_score = 0.5    # Track best score seen
        self.best_iteration = 0  # When best score was found
        self.history: List[Dict] = []

        # Pre-load EPD positions (once, not every iteration)
        self.test_positions = self._load_epd_positions()

    def _load_epd_positions(self) -> List[str]:
        """Load test positions from EPD file."""
        # Try to find EPD file
        epd_path = os.path.join(os.path.dirname(__file__), "quiet-labeled.epd")
        if not os.path.exists(epd_path):
            epd_path = os.path.join(os.path.dirname(self.engine_path), "..", "tuner", "quiet-labeled.epd")

        positions = []

        if os.path.exists(epd_path):
            try:
                print(f"Loading positions from {epd_path}...")
                with open(epd_path, 'r') as f:
                    # Read up to fen_count lines
                    for i, line in enumerate(f):
                        if i >= self.fen_count:
                            break
                        line = line.strip()
                        if line:
                            # EPD format: FEN c9 "result"; - extract just the FEN part
                            parts = line.split(' c9 ')
                            if parts:
                                fen = parts[0].strip()
                                fen_parts = fen.split()
                                if len(fen_parts) == 4:
                                    fen += " 0 1"
                                positions.append(fen)
                print(f"Loaded {len(positions)} positions")
            except Exception as e:
                print(f"Warning: Could not load EPD file: {e}")

        # Fallback to hardcoded positions
        if not positions:
            positions = [
                "r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4",
                "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3",
                "rnbqkb1r/pp2pppp/3p1n2/2p5/4P3/2N2N2/PPPP1PPP/R1BQKB1R w KQkq - 0 4",
                "r1bqkbnr/pppp1ppp/2n5/1B2p3/4P3/5N2/PPPP1PPP/RNBQK2R b KQkq - 3 3",
                "rnbqkb1r/pppppppp/5n2/8/2PP4/8/PP2PPPP/RNBQKBNR b KQkq - 0 2",
            ]

        return positions

    def _get_spsa_coefficients(self, k: int) -> Tuple[float, float]:
        """Calculate SPSA a_k and c_k for iteration k."""
        # a_k = a / (A + k + 1)^alpha
        # c_k = c / (k + 1)^gamma
        a_k = 1.0 / ((self.A + k + 1) ** self.alpha)
        c_k = 1.0 / ((k + 1) ** self.gamma)
        return a_k, c_k

    def _generate_perturbation(self) -> List[int]:
        """Generate random perturbation vector (+1 or -1 for each parameter)."""
        return [2 * random.randint(0, 1) - 1 for _ in self.params]

    def _build_engine_options(self, perturbed_values: Dict[str, float]) -> str:
        """Build UCI setoption commands string."""
        options = []
        for name, value in perturbed_values.items():
            options.append(f"option.{name}={int(value)}")
        return " ".join(options)

    def _run_games(self, params_plus: Dict[str, float], params_minus: Dict[str, float]) -> Tuple[float, float]:
        """
        Run games between two parameter sets.

        Returns:
            (score_plus, score_minus): Scores as ratio [0, 1]
        """
        # Build option strings
        opts_plus = self._build_engine_options(params_plus)
        opts_minus = self._build_engine_options(params_minus)

        half_games = self.games_per_iter // 2

        # Run games: plus vs minus
        cmd = [
            self.cutechess_path,
            "-engine", f"cmd={self.engine_path}", opts_plus, "name=Plus",
            "-engine", f"cmd={self.engine_path}", opts_minus, "name=Minus",
            "-each", f"tc={self.time_control}", "proto=uci",
            "-games", str(half_games),
            "-rounds", str(half_games),
            "-concurrency", str(self.concurrency),
            "-pgnout", "spsa_games.pgn",
            "-recover",
            "-wait", "100",
        ]

        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=3600)
            output = result.stdout

            # Parse results
            wins_plus = 0
            draws = 0
            wins_minus = 0

            for line in output.split('\n'):
                if 'Score of Plus vs Minus' in line:
                    # Format: "Score of Plus vs Minus: X - Y - Z [ratio]"
                    parts = line.split(':')[1].strip().split()
                    wins_plus = int(parts[0])
                    wins_minus = int(parts[2])
                    draws = int(parts[4])
                    break

            total = wins_plus + wins_minus + draws
            if total == 0:
                return 0.5, 0.5

            score_plus = (wins_plus + draws * 0.5) / total
            score_minus = (wins_minus + draws * 0.5) / total

            return score_plus, score_minus

        except subprocess.TimeoutExpired:
            print("Warning: Game timeout, returning draw")
            return 0.5, 0.5
        except Exception as e:
            print(f"Error running games: {e}")
            return 0.5, 0.5

    def _run_games_simple(self, params_plus: Dict[str, float], params_minus: Dict[str, float]) -> Tuple[float, float]:
        """
        Simple self-play without cutechess.
        Uses engine's internal evaluation across multiple positions.

        Returns:
            (score_plus, score_minus): Estimated scores based on evaluation
        """
        # Sample random positions from pre-loaded set
        sample_size = min(self.positions_per_iter, len(self.test_positions))
        TEST_POSITIONS = random.sample(self.test_positions, sample_size)

        def get_eval(params: Dict[str, float], fen: str) -> float:
            """Run engine and get evaluation of a position."""
            cmd = [self.engine_path]
            opts = "\n".join([f"setoption name {k} value {int(v)}" for k, v in params.items()])

            # Use 'fen' for FEN positions
            pos_cmd = f"position fen {fen}"
            input_str = f"uci\n{opts}\nisready\n{pos_cmd}\ngo depth {self.search_depth}\n"

            try:
                proc = subprocess.run(
                    cmd,
                    input=input_str,
                    capture_output=True,
                    text=True,
                    timeout=15
                )

                # Parse score from output (get last depth's score)
                for line in reversed(proc.stdout.split('\n')):
                    if 'score cp' in line:
                        parts = line.split('score cp ')
                        if len(parts) > 1:
                            score = int(parts[1].split()[0])
                            return score
                    elif 'score mate' in line:
                        parts = line.split('score mate ')
                        if len(parts) > 1:
                            mate = int(parts[1].split()[0])
                            return 10000 if mate > 0 else -10000
                return 0
            except:
                return 0

        # Get evaluations for both parameter sets across all positions
        total_diff = 0.0
        positions_tested = 0

        for fen in TEST_POSITIONS:
            eval_plus = get_eval(params_plus, fen)
            eval_minus = get_eval(params_minus, fen)

            # Accumulate signed difference (plus - minus)
            diff = eval_plus - eval_minus
            total_diff += diff
            positions_tested += 1

        # Average difference across positions
        avg_diff = total_diff / max(1, positions_tested)

        # Convert eval difference to expected score (logistic function)
        # Using 100cp scaling for sensitivity
        score_plus = 1 / (1 + 10 ** (-avg_diff / 100))  # More sensitive scaling
        score_minus = 1 - score_plus

        return score_plus, score_minus

    def iterate(self, use_simple_games: bool = False) -> Dict:
        """
        Run one SPSA iteration.

        Args:
            use_simple_games: Use simple evaluation comparison instead of full games

        Returns:
            Dictionary with iteration results
        """
        self.iteration += 1
        k = self.iteration

        # Get SPSA coefficients
        a_k, c_k = self._get_spsa_coefficients(k)

        # Generate perturbation
        delta = self._generate_perturbation()

        # Create perturbed parameter sets
        params_plus = {}
        params_minus = {}

        for i, param in enumerate(self.params):
            perturbation = c_k * param.c_end * delta[i]
            params_plus[param.name] = param.clamp(param.value + perturbation)
            params_minus[param.name] = param.clamp(param.value - perturbation)

        # Run games
        if use_simple_games:
            score_plus, score_minus = self._run_games_simple(params_plus, params_minus)
        else:
            score_plus, score_minus = self._run_games(params_plus, params_minus)

        # Calculate gradient and update parameters
        for i, param in enumerate(self.params):
            if delta[i] != 0:
                gradient = (score_plus - score_minus) / (2 * c_k * param.c_end * delta[i])
                step = a_k * param.a_end * gradient
                param.value = param.clamp(param.value + step)

        # Update current/final params
        self.current_params = {p.name: p.value for p in self.params}

        # Track best score (when plus score is significantly better)
        combined_score = score_plus
        if combined_score > self.best_score:
            self.best_score = combined_score
            self.best_params = dict(self.current_params)
            self.best_iteration = k

        # Record history
        result = {
            "iteration": k,
            "score_plus": score_plus,
            "score_minus": score_minus,
            "best_score": self.best_score,
            "best_iteration": self.best_iteration,
            "params": dict(self.current_params),
        }
        self.history.append(result)

        return result

    def run(self, iterations: int, use_simple_games: bool = False) -> Dict[str, float]:
        """
        Run SPSA tuning for specified iterations.

        Args:
            iterations: Number of SPSA iterations
            use_simple_games: Use simple evaluation comparison

        Returns:
            Dictionary of optimized parameter values
        """
        print(f"Starting SPSA tuning for {iterations} iterations...")
        print(f"Engine: {self.engine_path}")
        print(f"Parameters: {[p.name for p in self.params]}")
        print()

        for i in range(iterations):
            result = self.iterate(use_simple_games)

            print(f"[{i+1}/{iterations}] Plus: {result['score_plus']:.3f}, "
                  f"Minus: {result['score_minus']:.3f}, Best: {self.best_score:.3f} @ iter {self.best_iteration}")

            # Print current best values every 10 iterations
            if (i + 1) % 10 == 0:
                print("\nCurrent parameter values:")
                for p in self.params:
                    print(f"  {p.name}: {int(p.value)}")
                print()

        return self.current_params  # Return final params after all iterations

    def save_results(self, filename: str = "spsa_results.json"):
        """Save tuning results to JSON file."""
        output = {
            "summary": {
                "total_iterations": self.iteration,
                "best_score": round(self.best_score, 4),
                "best_iteration": self.best_iteration,
            },
            "final_params": {k: int(v) for k, v in self.current_params.items()},
            "best_params": {k: int(v) for k, v in self.best_params.items()},
            "initial_params": {p.name: int(p.min_val + (p.max_val - p.min_val) / 2) for p in DEFAULT_PARAMS},
            "history": self.history,
        }
        with open(filename, 'w') as f:
            json.dump(output, f, indent=2)
        print(f"Results saved to {filename}")

    def generate_cpp_output(self) -> str:
        """Generate C++ code snippet with tuned values."""
        lines = [
            "// ============================================================================",
            "// SPSA Tuned Values (Best Parameters)",
            "// Generated by spsa_tuner.py",
            f"// Best Score: {self.best_score:.4f} at iteration {self.best_iteration}",
            "// ============================================================================",
            "",
        ]

        for name, value in self.best_params.items():
            lines.append(f"// constexpr int {name} = {int(value)};")

        return "\n".join(lines)


# ============================================================================
# Main Entry Point
# ============================================================================

def main():
    parser = argparse.ArgumentParser(description="SPSA Tuner for Chess Engine")
    parser.add_argument("--engine", required=True, help="Path to engine executable")
    parser.add_argument("--iterations", type=int, default=100, help="Number of SPSA iterations")
    parser.add_argument("--fen-count", type=int, default=500, help="Number of FEN positions to load from EPD")
    parser.add_argument("--positions-per-iter", type=int, default=20, help="Positions to sample per iteration")
    parser.add_argument("--search-depth", type=int, default=4, help="Search depth for evaluation")
    parser.add_argument("--games-per-iter", type=int, default=100, help="Games per iteration (full mode)")
    parser.add_argument("--time-control", default="1+0.1", help="Time control (e.g., 1+0.1)")
    parser.add_argument("--concurrency", type=int, default=1, help="Concurrent games")
    parser.add_argument("--cutechess", default="cutechess-cli", help="Path to cutechess-cli")
    parser.add_argument("--simple", action="store_true", help="Use simple eval comparison (no cutechess)")
    parser.add_argument("--output", default="spsa_results.json", help="Output file")

    args = parser.parse_args()

    # Check engine exists
    if not os.path.exists(args.engine):
        print(f"Error: Engine not found: {args.engine}")
        sys.exit(1)

    # Create tuner with configurable parameters
    tuner = SPSATuner(
        engine_path=args.engine,
        params=[TunableParam(p.name, p.value, p.min_val, p.max_val) for p in DEFAULT_PARAMS],
        games_per_iter=args.games_per_iter,
        time_control=args.time_control,
        concurrency=args.concurrency,
        cutechess_path=args.cutechess,
        fen_count=args.fen_count,
        positions_per_iter=args.positions_per_iter,
        search_depth=args.search_depth,
    )

    # Run tuning
    best_params = tuner.run(args.iterations, use_simple_games=args.simple)

    # Save results
    tuner.save_results(args.output)

    # Print final values
    print("\n" + "=" * 60)
    print("FINAL TUNED VALUES (after all iterations):")
    print("=" * 60)
    for name, value in best_params.items():
        print(f"{name}: {int(value)}")

    print("\n" + "=" * 60)
    print(f"BEST VALUES (from iteration {tuner.best_iteration}, score: {tuner.best_score:.4f}):")
    print("=" * 60)
    for name, value in tuner.best_params.items():
        print(f"{name}: {int(value)}")

    print("\n" + tuner.generate_cpp_output())

    return 0


if __name__ == "__main__":
    sys.exit(main())
