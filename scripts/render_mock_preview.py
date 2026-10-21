"""Static layout proof; not a JUCE UI capture. Safe to delete."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import math
r=Path(__file__).resolve().parents[1]
folder=r/'Assets'
bg=Image.new('RGBA',(956,984),(4,4,5,255))
d=ImageDraw.Draw(bg)
d.rectangle((0,586,956,984),fill=(12,13,16,255))
bg.alpha_composite(Image.open(folder/'fan_body.png').convert('RGBA'))
rot=Image.open(folder/'blades.png').convert('RGBA').rotate(24,resample=Image.Resampling.BICUBIC)
bg.alpha_composite(rot,(166,85))
bg.alpha_composite(Image.open(folder/'hub_cap.png').convert('RGBA'),(371,290))
bg.alpha_composite(Image.open(folder/'grille.png').convert('RGBA'),(146,65))
d=ImageDraw.Draw(bg)
d.rounded_rectangle((218,729,734,917),19,fill=(25,125,189,255))
d.rounded_rectangle((232,738,720,908),14,fill=(185,190,194,255),outline=(105,110,115,255),width=2)
d.rounded_rectangle((0,0,955,59),1,fill=(12,12,14,255))
d.rectangle((0,58,956,60),fill=(38,38,40,255))
font='/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed.ttf'
bold='/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf'
def f(n,b=False):return ImageFont.truetype(bold if b else font,n)
def txt(x,y,t,size=12,c=(35,42,47,255),center=False,b=True):
 d.text((x,y),t,font=f(size,b),fill=c,anchor='ma' if center else None)
for i,(name,x,x2,col) in enumerate([('VIBRATO',246,299,(234,232,230,255)),('ROTARY',300,352,(46,196,242,255)),('TREMOLO',352,403,(58,166,238,255)),('CHORUS',403,457,(31,79,216,255))]):
 d.rounded_rectangle((x,808,x2,881),6,fill=col)
 txt((x+x2)//2,897,name,11,center=True)
for name,x,base in [('MAN',485,(150,208,202,255)),('HOST',523,(200,164,225,255)),('♩',565,(228,230,231,255)),('♩.',604,(225,225,227,255)),('♩³',641,(227,228,230,255))]:
 d.rounded_rectangle((x,752,x+34,782),4,fill=base)
 txt(x+17,766,name,12,center=True)
txt(355,758,'DEPTH',15,center=True)
d.line((259,779,451,779),fill=(63,65,68,255),width=3)
for i in range(44):
 x=258+i*4.5
 d.line((x,772,x,787),fill=(58,60,62,255),width=1)
d.rounded_rectangle((346,759,363,794),5,fill=(171,178,186,255),outline=(62,66,70,255))
for name,x,y,s in [('RATE',517,888,12),('WAVE',640,888,12),('SATURATION',674,720,11)]:txt(x,y,name,s,center=True)
for img,x,y,s in [('knob_chrome.png',477,800,80),('knob_chrome.png',600,800,80),('knob_blue.png',655,669,42)]:
 part=Image.open(folder/img).convert('RGBA');bg.alpha_composite(part,(x,y))
d=ImageDraw.Draw(bg)
for x,t in [(478,'8'),(555,'24'),(480,'0'),(556,'32')]:txt(x,820 if t in ['8','24'] else 866,t,12)
for x,y,t in [(595,817,'∿'),(598,864,'∧'),(680,817,'⊓'),(682,864,'⌁')]:txt(x,y,t,18)
d.rounded_rectangle((283,696,324,734),7,fill=(30,35,39,255))
txt(302,706,'↻',23,(240,240,242,255),center=True)
# control icons and text
for x,t in [(10,'⏻'),(56,'∿'),(546,'▣'),(583,'▤'),(621,'⤨')]:
 d.rounded_rectangle((x,9,x+34,49),3,fill=(29,30,34,255))
 txt(x+17,19,t,19,(232,237,238,255),center=True)
d.rounded_rectangle((243,10,539,48),3,fill=(14,14,16,255),outline=(55,55,59,255))
txt(394,25,'Chorus - Classic',16,(230,230,233,255),center=True,b=False)
txt(708,22,'W I D T H',12,(200,200,200,255),center=True)
txt(845,22,'D R Y / W E T',12,(200,200,200,255),center=True)
for x in [769,921]:
 d.ellipse((x-23,5,x+23,51),fill=(40,41,44,255),outline=(207,208,209,255),width=3)
 d.line((x,7,x-7,21),fill=(232,235,236,255),width=3)
d.rounded_rectangle((361,942,588,977),10,fill=(222,221,218,235))
txt(475,953,'B R E E Z E M O D',22,(30,34,39,255),center=True)
bg.convert('RGB').save(r/'BreezeMod-layout-preview.jpg',quality=92)
print('Preview created')
