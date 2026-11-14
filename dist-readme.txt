isaac-highfps - smooth high refresh rate for The Binding of Isaac: Repentance+
https://github.com/julenster/isaac-highfps

WINDOWS
  Double-click install.bat. Done.
  (To remove it: double-click uninstall.bat)

LINUX / STEAM DECK (desktop mode)
  Open a terminal in this folder and run:   sh install-linux.sh
  Then in Steam: right-click the game > Properties > General > Launch options:
      WINEDLLOVERRIDES="winmm=n,b" %command%
  (To remove it: sh install-linux.sh uninstall)

MANUAL INSTALL (any system)
  Copy winmm.dll into the game folder
  (Steam > right-click the game > Manage > Browse local files).
  Linux / Steam Deck also need the launch option above.

------------------------------------------------------------------------
ESPAÑOL
  Windows: doble clic en install.bat. Listo. (Para quitarlo: uninstall.bat)
  Linux / Steam Deck: en una terminal en esta carpeta:  sh install-linux.sh
  y en Steam > clic derecho > Propiedades > General > Opciones de lanzamiento:
      WINEDLLOVERRIDES="winmm=n,b" %command%
