# Residue colors in 0.14.1

Choose View → Residue palette or the toolbar swatch dropdown. Kleio preserves
the previously accepted colors. Alternative palettes keep letters as redundant
base identifiers; colors alone are not required to distinguish residues.

| Palette | A | C | G | T / U |
| --- | --- | --- | --- | --- |
| Kleio | #B47D00 | #0000FF | #000000 | #FF0000 |
| Chromas-style | #008040 | #0000FF | #000000 | #FF0000 |
| High contrast | #8B6500 | #0050A4 | #202020 | #A00070 |
| Okabe–Ito | #E69F00 | #0072B2 | #009E73 | #CC79A7 |
| Paul Tol bright | #CCBB44 | #4477AA | #228833 | #AA3377 |

These are the base colors used for cell backgrounds and swatches. High contrast
is a custom set. The other two alternatives select four colors from the original
[Okabe–Ito Color Universal Design palette](https://jfly.uni-koeln.de/color/) and
[Paul Tol bright scheme](https://sronpersonalpages.nl/~pault/). Assignment of these
colors to nucleotides is specific to Kleiotide.

For High contrast, Okabe–Ito and Paul Tol colored letters and trace channels, a base color is darkened in
5% RGB steps until its nominal contrast against white is at least 4.5:1. Thus the
letter/channel ink can be darker than the corresponding base background above.
Background mode chooses whichever of pure black or white has the larger contrast
ratio. Calculations use sRGB relative luminance and the
[W3C contrast formula](https://www.w3.org/WAI/WCAG22/Understanding/contrast-minimum.html).
Tests cover the nominal colors; this is not a claim of whole-application WCAG
conformance or identical perception for every form of color-vision deficiency.

Kleio and Chromas-style keep their base inks exact, including red #FF0000;
their letter colors are not subject to the 4.5:1 adaptation. Chromas-style uses
the four colors requested by the user; no external software dependency is added.

Kleio colored letters retain the existing ambiguity purple and light gap
colors. Other palettes give ambiguities, N and ? neutral gray styling;
gaps use neutral gray backgrounds in background mode. Monochrome uses near-black
letters and darker gray gaps. Masks remain dark gray with no residue cell fill.
Selections override cell backgrounds with the established blue/yellow editing
cues and dark symbols. Identity dots keep each actual residue's cell color.

Display choices alter rendering only. The selected palette also colors the four
trace channels and their legend. Monochrome and colored backgrounds apply to the
alignment grid; trace evidence labels retain their editing/provenance cues.
Overview conservation uses its independent blue/orange scale. New sessions save
both preferences as optional view metadata; FASTA contains only real symbols.
