
rd /s /q pack_folder
mkdir pack_folder
xcopy /S /Y /EXCLUDE:exclude.txt  F:\deviceManager\bin\win32\release .\pack_folder\
iscc utools_hm.iss
