How to create the HTML help
---------------------------

- latex export with LyX
- add \DeclareGraphicsExtensions{.png,.jpg} to latex file
- htlatex sdmvis-manual
- manually adapt image widht/height in html
- manually adjust css:
    body { font-family: sans-serif; font-size: 90%; }


- adapt qthelp.qhq
- create qch:
	qhelpgenerator qthelp.qhp -o qthelp.qch
- adapt qthelpcollection.qhcp
- create qch:
	qcollectiongenerator qthelpcollection.qhcp -o qthelpcollection.qhc


- on some systems .jpg's are not displayed
  therefore we should use only .png
  (adapted html output manually for now)