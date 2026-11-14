isaac-highfps - high FPS for The Binding of Isaac: Repentance+ (v1.9.7.17)
https://github.com/julenster/isaac-highfps

INSTALL
  1. Steam > right-click The Binding of Isaac > Manage > Browse local files
  2. Copy winmm.dll (and optionally highfps.ini) into that folder
  3. Linux / Steam Deck only: Properties > General > Launch options:
         WINEDLLOVERRIDES="winmm=n,b" %command%
  Done. Play as usual.

UNINSTALL
  Delete winmm.dll (and highfps.ini, highfps.log). On Linux, clear the launch option.

ESPAÑOL
  Instalar: copia winmm.dll en la carpeta del juego (Steam > clic derecho >
  Administrar > Ver archivos locales). En Linux / Steam Deck añade además en
  Opciones de lanzamiento:  WINEDLLOVERRIDES="winmm=n,b" %command%
  Desinstalar: borra winmm.dll.
