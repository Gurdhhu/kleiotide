# Kleiotide icon assets

For the current Windows/Qt build, keep an editable master and a multi-image ICO.

The official designer credit and project permission are recorded in BRANDING.md:
Elizaveta Shchepina (Storminka / Io Tikhaya). Supply the artwork later under that
policy; asset format conversion preserves the design and its credit.

| Asset | Recommended specification |
| --- | --- |
| Editable master | SVG with a square viewBox; outline any text and embed linked artwork. |
| Raster alternative | 1024 × 1024 transparent PNG, RGBA, sRGB. This is a convenient working master, not a Windows requirement. |
| Windows application icon | One .ico containing 32-bit color images with alpha transparency at 16, 20, 24, 32, 40, 48, 64, 128 and 256 pixels square. |
| Minimum ICO variants | Microsoft specifies at least 16, 24, 32, 48 and 256 pixels square. |
| DPI metadata | Pixel dimensions determine the asset size. Use 96 DPI if an editor asks; 300 DPI is unnecessary. Export the exact pixel dimensions without resampling merely to change DPI metadata. |

The expanded size list above is a practical recommendation. Optional 30, 36, 60,
72, 80 and 96 pixel variants can further reduce Windows scaling. Inspect the
16/24/32 pixel images at actual size; simplify small versions if fine detail or
text becomes illegible. Use a transparent background and check both light and
dark surroundings. Keep the master rather than enlarging a small exported icon.

Qt uses an ICO embedded through a Windows resource file for the executable icon;
the application/window icon is also set through Qt. These are integration steps
after the artwork is ready. This patch does not add placeholder artwork.

References:

- [Microsoft: Construct your Windows app's icon](https://learn.microsoft.com/en-us/windows/apps/design/iconography/app-icon-construction) — required sizes, scaling and transparency.
- [Microsoft: About icons](https://learn.microsoft.com/en-us/windows/win32/menurc/about-icons) — multi-image ICO, 32-bit alpha and display-DPI behavior.
- [Qt 6.8: Setting the application icon](https://doc.qt.io/qt-6.8/appicon.html) — Windows ICO/resource integration and window icon.
