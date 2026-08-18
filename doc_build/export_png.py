"""Exporta cada slide de um .pptx para PNG usando o PowerPoint instalado.

Usado no lugar do caminho LibreOffice+pdftoppm da skill pptx, que nao esta
disponivel nesta maquina. Renderiza com as fontes reais do Office, entao a
checagem de overflow e mais fiel que a do LibreOffice.

    python export_png.py deck.pptx saida_dir [largura_px]
"""
import os
import sys
import glob

import win32com.client

deck = os.path.abspath(sys.argv[1])
outdir = os.path.abspath(sys.argv[2])
width = int(sys.argv[3]) if len(sys.argv) > 3 else 1600
height = int(width * 9 / 16)

os.makedirs(outdir, exist_ok=True)
for old in glob.glob(os.path.join(outdir, '*.png')):
    os.remove(old)

app = win32com.client.Dispatch('PowerPoint.Application')
pres = None
try:
    # WithWindow=False falha em algumas builds; abre normal e fecha depois.
    pres = app.Presentations.Open(deck, ReadOnly=True, WithWindow=False)
    for i, slide in enumerate(pres.Slides, 1):
        slide.Export(os.path.join(outdir, 'slide-%02d.png' % i), 'PNG', width, height)
    print('exportados %d slides em %s' % (pres.Slides.Count, outdir))
finally:
    if pres is not None:
        pres.Close()
    app.Quit()
