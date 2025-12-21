# Roadmap Lengkap Pengembangan Chess Engine

## Target: Strong/Master Level (2400-2600 ELO)

Dokumen ini berisi roadmap teknis untuk membangun mesin catur yang kompetitif. Fokus pada performansi, kekuatan ELO, dan best practices modern.

---

## Tahap 0: Foundation & Infrastructure (Estimasi Impact: Baseline)

**Setup Project**

- [ ] Struktur project yang modular (pisahkan board, search, eval, UCI)
- [ ] Unit testing framework (minimal untuk move generation & evaluation)
- [ ] Perft testing (debugging move generation dengan node count)
- [ ] Benchmarking suite (simpan posisi-posisi sulit untuk testing)
- [ ] Logging system untuk debugging search

**Basic Move Generation**

- [ ] Pseudo-legal move generation yang lengkap (semua piece types)
- [ ] Legality checking (pastikan king tidak dalam skak)
- [ ] Special moves: castling, en passant, promotion
- [ ] FEN parsing dan board representation

---

## Tahap 1: High-Performance Representation (Impact: +500 ELO) ✅ COMPLETED

**Wajib untuk Speed - Array biasa terlalu lambat**

**Bitboards (64-bit Integers)**

- [x] Representasikan setiap jenis piece sebagai 64-bit integer
- [x] Implementasi operasi bitwise: AND, OR, XOR, NOT, Shift (<<, >>)
- [x] Fungsi utilitas:
  - [x] Popcount (hitung jumlah bit 1) - gunakan compiler intrinsics
  - [x] LSB/MSB (Least/Most Significant Bit)
  - [x] Bit scanning (find first set bit)

**Magic Bitboards (Impact: +100 ELO)**

- [x] Magic numbers untuk Rook dan Bishop
- [x] Lookup tables untuk sliding pieces dalam O(1)
- [x] Occupy bitboard untuk blocking pieces
- [x] Implementasi efficient attack generation

**Zobrist Hashing (Critical)**

- [x] Generate random 64-bit numbers untuk:
  - [x] Setiap piece di setiap square (12 × 64)
  - [x] Castling rights (4 kemungkinan)
  - [x] En passant file (8 file)
  - [x] Side to move
- [x] Incremental hash update (XOR saat move/unmove)
- [x] Deteksi repetisi menggunakan hash history

---

## Tahap 2: Advanced Search & Move Ordering (Impact: +600-800 ELO) ✅ COMPLETED

**Principal Variation Search (PVS)**

- [x] Implementasi null window search (alpha, alpha+1)
- [x] Re-search dengan full window jika fail-high
- [x] PV (Principal Variation) tracking dan reporting

**Move Ordering (Sangat Kritis! Impact: +300 ELO)**

_Urutan prioritas pengurutan:_

1. [x] **Hash Move**: Best move dari Transposition Table di depth sebelumnya
2. [x] **Captures dengan SEE**:
   - [x] Static Exchange Evaluation (SEE) untuk menilai capture
   - [x] MVV-LVA sebagai fallback sederhana
3. [x] **Killer Moves**: 2 killer moves per ply (langkah yang menyebabkan cutoff)
4. [x] **Counter Move**: Respon optimal terhadap opponent's last move
5. [x] **History Heuristic**:
   - [x] History table [from_square][to_square]
   - [x] Butterfly boards
   - [x] Bonus untuk cutoff moves
6. [x] **Quiet moves** lainnya (sorted by history)

**Transposition Table (TT) (Impact: +200 ELO)**

- [x] Simpan: Hash key, Score, Depth, Bound type, Best move, Age
- [x] Bound types: EXACT, LOWERBOUND (beta cutoff), UPPERBOUND (all nodes)
- [x] Replacement strategy:
  - [x] Depth-preferred replacement
  - [x] Always replace dari search yang lebih lama (age)
  - [x] Two-tier system (shallow + deep)
- [x] Size: alokasikan 64-256 MB minimal
- [x] TT probe di awal node
- [x] TT store sebelum return dari node

**Internal Iterative Deepening (IID)**

- [x] Cari dengan depth rendah dulu jika tidak ada hash move
- [x] Gunakan hasil pencarian sebagai hash move untuk full-depth search

---

## Tahap 3: Selective Search - Pruning & Extensions (Impact: +400-600 ELO) ✅ COMPLETED

**Must-Have Pruning Techniques:**

**Quiescence Search (Wajib! Impact: +300 ELO)**

- [x] Search captures sampai posisi tenang
- [x] Delta pruning (skip capture jika material terlalu rendah)
- [x] SEE pruning dalam qsearch
- [x] Standing pat (return static eval jika >= beta)

**Null Move Pruning (Impact: +150 ELO)**

- [x] R = 3 + depth/4 + dynamic adjustment (reduction depth)
- [x] Jangan lakukan di:
  - [x] Posisi dalam check
  - [x] Endgame dengan hanya pawn (hasNonPawnMaterial check)
  - [x] Setelah null move sebelumnya (double null move pruning)
- [x] Verification search untuk zugzwang (at depth >= 12)

**Late Move Reduction (LMR) (Impact: +150 ELO)**

- [x] Reduce depth untuk moves setelah move ke-4 atau ke-5
- [x] Jangan reduce:
  - [x] Hash move
  - [x] Captures
  - [x] Promotions
  - [x] Moves yang memberi check
  - [x] Moves di PV node
- [x] Formula: reduction = base + log(depth) × log(move_number)
- [x] Re-search dengan full depth jika hasil mengejutkan

**Razoring**

- [x] Return qsearch jika depth rendah dan eval jauh di bawah alpha (table-based margins)

**Reverse Futility Pruning (Static Null Move)**

- [x] Beta cutoff jika eval >> beta dan depth rendah (RFPMargin table)

**Futility Pruning**

- [x] Skip quiet moves di near-leaf nodes jika eval + margin < alpha (FutilityMargin table)

**Late Move Pruning (LMP)**

- [x] Skip late quiet moves at low depths (LMPThreshold table)

**SEE Pruning in Main Search**

- [x] Skip losing captures at low depths
- [x] Skip quiet moves with very negative SEE

**Multi-Cut Pruning**

- [x] Integrated dengan singular extension: return singularBeta jika >= beta

**Extensions (Critical untuk Tactical Strength):**

**Check Extensions (Impact: +50 ELO)**

- [x] Extend depth +1 saat king dalam check
- [x] Limit: max 16 extensions per path (MAX_EXTENSIONS)

**Singular Extensions**

- [x] Extend hash move jika jauh lebih baik dari alternatives
- [x] Reduced search untuk verify singularity (SINGULAR_DEPTH = 6)
- [x] Multi-cut early return jika singularBeta >= beta

**Passed Pawn Extensions**

- [x] Extend untuk pawn yang maju ke rank 7 (near promotion)

**Aspiration Windows**

- [x] Search dengan window sempit di root: (alpha-20, beta+20)
- [x] Re-search dengan window lebih lebar jika fail
- [x] Gradual widening dengan delta += delta / 2

---

## Tahap 4: Modern Evaluation (Pilih Salah Satu)

### Opsi A: Hand-Crafted Evaluation (HCE) - Klasik & Solid (2200-2500 ELO) ✅ COMPLETED

**Material & Basic Evaluation**

- [x] Nilai material baseline: P=100, N=320, B=330, R=500, Q=950 (MG/EG values)
- [x] Piece-Square Tables (PST) untuk positioning (separate MG/EG tables)
- [x] Tapered evaluation: interpolasi Opening → Endgame (phase-based)

**Advanced Evaluation Terms:**

**Pawn Structure (Impact: +100 ELO)**

- [x] Passed pawns (bonus berdasarkan rank - PassedPawnBonus table)
- [x] Backward pawns (penalty - BackwardPawnPenalty)
- [x] Doubled pawns (penalty - DoubledPawnPenalty)
- [x] Isolated pawns (penalty - IsolatedPawnPenalty)
- [x] Pawn chains dan phalanx (bonus - ConnectedPawnBonus, PhalanxBonus)
- [ ] Candidate passed pawns (optional enhancement)
- [ ] Pawn storms (optional enhancement)

**Piece Activity (Impact: +80 ELO)**

- [x] Mobility (jumlah legal moves per piece - mobility tables per piece type)
- [x] Outposts (Knight di square yang dilindungi pawn - KnightOutpostBonus)
- [x] Rook on open/semi-open files (RookOpenFileBonus, RookSemiOpenFileBonus)
- [x] Rook on 7th rank (RookOnSeventhBonus)
- [x] Bishop pair bonus (BishopPairBonus)
- [ ] Trapped pieces (penalty) - optional enhancement

**King Safety (Impact: +100 ELO)**

- [x] Pawn shield (pawns di depan castled king - PawnShieldBonus)
- [ ] Pawn storm oleh lawan (optional enhancement)
- [x] Attack weight per piece type yang menyerang (Knight/Bishop/Rook/Queen weights)
- [x] King zone (squares di sekitar king - 3x3 area)
- [x] King safety table (attack units → penalty lookup)

**Automated Tuning (Optional for Future)**

- [ ] Texel Tuning: optimize parameters menggunakan dataset jutaan posisi
- [ ] SPSA (Simultaneous Perturbation Stochastic Approximation)
- [ ] Prepare quiet position dataset (EPD format)

### Opsi B: NNUE (Efficiently Updatable Neural Network) (2400-2600+ ELO)

**Implementasi NNUE**

- [ ] Arsitektur: (768 → 256) × 2 → 32 → 1 (standard architecture)
- [ ] Feature set: HalfKP (King position + piece position)
- [ ] Incremental update (add/remove features saat move)
- [ ] SIMD optimizations (AVX2/AVX-512)
- [ ] Load pretrained network (.nnue file)

**Data Generation untuk Training (Opsional)**

- [ ] Generate training data: jutaan posisi dengan eval + result
- [ ] Format dataset: binpack atau plain-text
- [ ] Self-play untuk generate data
- [ ] Training pipeline dengan PyTorch/TensorFlow

---

## Tahap 5: Opening & Endgame Knowledge ✅ COMPLETED

**Opening Book (Impact: +50-100 ELO)**

- [x] Polyglot opening book format (.bin) - full implementation
- [x] Book moves dengan weight/priority
- [x] Variety mode (pilih random dari top moves berdasarkan weight)
- [x] Binary search untuk fast lookup
- [ ] Book learning (update book berdasarkan hasil game) - optional

**Endgame Tablebases (Impact: +50 ELO di endgame)**

- [x] Syzygy tablebase interface (siap untuk integrasi Fathom)
- [x] Probe saat piece count <= threshold (can_probe check)
- [x] DTZ (Distance to Zero) vs WDL (Win/Draw/Loss) interface
- [x] Root probe untuk perfect endgame play
- [ ] Full Fathom library integration (requires external library)

**Built-in Endgame Knowledge**

- [x] Known draw detection (KvK, KNvK, KBvK, KNNvK, opposite bishops)
- [x] Scale factor for drawish endgames (no pawns = 25%, one side no pawns = 50%)

---

## Tahap 6: UCI Protocol & Time Management (Critical untuk Tournament Play) ✅ COMPLETED

**UCI Protocol Lengkap**

- [x] Command: `uci`, `isready`, `position`, `go`, `stop`, `quit`
- [x] Options: `Hash`, `Threads`, `Ponder`, `MultiPV`, `Move Overhead`, `Book File`, `SyzygyPath`
- [x] Info output: `depth`, `seldepth`, `score`, `nodes`, `pv`, `time`, `nps`, `hashfull`
- [x] Ponder move output (`bestmove <move> ponder <move>`)
- [x] Command: `setoption name <name> value <value>`
- [x] Command: `ucinewgame` (reset TT and history)
- [x] Debug commands: `d` (display board), `eval`, `perft`
- [ ] Currmovenumber reporting (optional enhancement)

**Time Management (Impact: +100 ELO)**

- [x] Alokasi waktu base: time_left / 40 (default moves to go)
- [x] Stability-based time adjustment:
  - [x] Increase time if score drops significantly
  - [x] Increase time if best move keeps changing
  - [x] Decrease time if best move is stable
- [x] Increment handling (adds 75% of increment to base time)
- [x] Soft time (optimalTime) vs hard time limit (maximumTime)
- [x] Move overhead configuration
- [ ] Emergency time mode (optional enhancement)

---

## Tahap 7: Testing & Validation (Critical!) ✅ COMPLETED

**Perft Testing**

- [x] Test suite untuk verify move generation correctness
- [x] Check against known perft numbers (depth 6-7)
- [x] Perft divide untuk debugging move generation

**SPRT Testing (Sequential Probability Ratio Test)**

- [x] Setup cutechess-cli atau fastchess (run_sprt.bat)
- [x] Test: New version vs Baseline (script generator)
- [x] Parameters:
  - [x] Elo0 = 0, Elo1 = 5 (detect +5 ELO improvement)
  - [x] Alpha = 0.05, Beta = 0.05
- [x] Time control: 10+0.1 atau 5+0.05 (thousands of games)
- [x] Accept H1 (improvement) atau H0 (no improvement)
- [x] SPRT calculator function

**Benchmark Positions**

- [x] Tactical test suites (Win At Chess, Bratko-Kopec)
- [x] Strategic test suites (STS sample)
- [x] Track nodes/second (NPS) performance
- [x] Depth, time, and node-based benchmarks
- [x] Mate puzzle test suite

**Regression Testing**

- [x] Simpan versi sebelumnya sebagai baseline (run_sprt.bat)
- [x] Test setiap commit dengan gauntlet matches (run_gauntlet.bat)
- [x] Benchmark comparison function

---

## Tahap 8: Parallelism (Impact: +100-200 ELO untuk multi-core) ✅ COMPLETED

**Lazy SMP**

- [x] Multiple threads berbagi Transposition Table
- [x] Minimal synchronization (hence "lazy")
- [x] Setiap thread menjalankan full search
- [x] Random small depth perturbations untuk diversity
- [x] Helper threads membantu principal thread

**Thread Coordination**

- [x] Master thread koordinasi stop signal
- [x] Bestmove collection dari semua threads
- [x] Thread pool management
- [x] UCI option: "Threads" (1-256)

---

## Tahap 9: Advanced Techniques (Optional, for 2600+ ELO)

**Multi-PV Mode**

- [ ] Report multiple best lines (MultiPV=3 untuk top 3 moves)

**Syzygy/Gaviota Tablebase Probing**

- [ ] DTZ50 tables
- [ ] WDL tables

**Contempt Factor**

- [ ] Slight bonus untuk draw avoidance (saat rating lebih tinggi)

**Monte Carlo Tree Search (MCTS) Hybrid**

- [ ] Combine alpha-beta dengan MCTS untuk complex positions

---

## Resource & Tools

**Development Tools:**

- Cutechess-CLI: tournament management
- Fastchess: faster tournament testing
- Lichess Bot API: online testing
- Python-chess: untuk prototyping
- Chess Programming Wiki (chessprogramming.org)

**Testing Suites:**

- Silver suite (tactical test)
- Bratko-Kopec test
- Strategic Test Suite (STS)
- Arasan test suite

**Reference Engines (Open Source):**

- Stockfish (C++, terkuat)
- Ethereal (C, bagus untuk belajar)
- Berserk (C, simple tapi kuat)
- Vice (tutorial engine)

---

## Tips Implementasi

1. **Incremental Development:** Implementasikan satu fitur pada satu waktu, test dengan SPRT
2. **Measure Everything:** Jangan assume, ukur dengan data statistik
3. **Start Simple:** Mulai dengan Minimax → Alpha-Beta → Improve dari sana
4. **Don't Guess Parameters:** Gunakan automated tuning (Texel, SPSA)
5. **Study Open Source:** Baca kode Stockfish, Ethereal untuk best practices
6. **Focus on Move Ordering:** 80% kekuatan search berasal dari ordering yang baik
7. **Profiling:** Gunakan profiler untuk identifikasi bottleneck

---

**Good luck building your engine! 🚀♟️**
