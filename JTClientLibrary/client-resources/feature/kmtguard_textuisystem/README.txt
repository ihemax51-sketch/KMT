KMTGuard TextUISystem language resources

Choose one ready-to-use file:

- textuisystem_kmtguard_turkish.txt: Turkish only.
- textuisystem_kmtguard_english.txt: English only.

Append or import every row from the selected file into the client's
Media\server_dep\silkroad\textdata\textuisystem.txt table. Do not import both
single-language files together because they contain the same keys.

The client DLL reads the UIIT_KMT_* keys at runtime. Edit the language columns
in that text table to change KMTGuard client wording without rebuilding the DLL.

textuisystem_kmtguard.txt remains available as the combined English/Turkish
edition. Do not replace the server's complete TextUISystem file with any of
these partial files.
