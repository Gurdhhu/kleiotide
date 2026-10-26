# Kleiotide 0.21.1 acceptance

Extract into a fresh folder and build with your usual Qt >=6.5 Desktop/MSVC kit.
CMake automatically generates the shared application/export version header.

1. Calculate overall, within-group and between-group distances. SD (pairs) and
   SE (naive) appear beside Mean distance. For m valid pair distances, compare
   against spreadsheet STDEV.S over those pair values and SD/SQRT(m). Exclude
   the diagonal, duplicate unordered pairs and every N/A. Between-group means
   use each cross-group pair once. SD uses corrected distances for corrected models.
2. With only one valid pair, the mean is available but SD/SE are N/A. With zero
   valid pairs all three are N/A. At least two equal distances give SD=SE=0.
3. Check the visible explanation and column/cell tooltips: sample divisor m−1;
   naive SE assumes independent pairs, which shared-sequence pairs do not satisfy.
   It is not a bootstrap or dependence-adjusted uncertainty estimate.
4. Copy and export mean TSV: both include Mean, SD, SE, valid/excluded counts,
   formulas, denominator/limitations and Kleiotide version 0.21.1. Also check the
   version header in pairwise-matrix and nucleotide-composition Copy/Export TSV.
5. Recheck group scopes, corrected-model saturation, missing data, cancellation,
   sort/Undo and existing export cancellation. Test normal and 150% scaling.

Native sessions remain format 4. This patch does not implement bootstrap errors.
