# Version 0.14.1 — requested palette and overview changes

Build from a fresh Kleiotide_0.14.1 folder with the existing Qt/MSVC kit.

1. The default palette reads Kleio in View and the toolbar popup. Open a 0.14
   session using the old default: it still restores the same colors under Kleio.
2. Choose Chromas-style. A/a is #008040, C/c blue #0000FF, G/g black #000000,
   T/t and U/u red #FF0000. The letters and trace channels use these exact inks.
   Background mode uses those same fills with automatic black/white lettering.
3. Menu/toolbar selection, identity icons and trace legend stay synchronized.
   Save/reopen .kleio with Chromas-style: the palette and display style return.
   Other palettes, masks, selections and clipboard text retain their behavior.
4. The overview reads: Color: conserved column (blue) -> variable (orange),
   gray = no information. Its hover explanation uses the same text; the underlying
   conservation calculation and colors are unchanged.

ROADMAP_1.0.md reconciles completed work and the remaining release stages.
