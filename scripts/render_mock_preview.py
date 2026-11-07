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
# v1.4's compact, vector-rendered live panels. This PNG is a layout concept,
# not a capture of the JUCE plugin in a host.
for x1,y1,x2,y2 in [(25,743,141,926),(778,743,933,926)]:
 d.rounded_rectangle((x1,y1,x2,y2),12,fill=(12,22,30,255),outline=(52,84,101,255),width=2)
 d.line((x1+12,y1+29,x2-12,y1+29),fill=(41,67,83,255),width=1)
txt(83,751,'OUTPUT',14,(202,232,247,255),center=True)
for j,x in enumerate([51,94]):
 txt(x+12,772,'L' if j==0 else 'R',12,(170,192,209,255),center=True)
 for i in range(21):
  y=787+(20-i)*5
  on=i<(14 if j==0 else 17)
  c=((235,103,90,255) if i>=19 else (235,198,101,255) if i>=16 else (60,180,233,255))
  if not on:c=(c[0]//7,c[1]//7,c[2]//7,255)
  d.rounded_rectangle((x,y,x+24,y+4),1,fill=c)
 d.rounded_rectangle((x+5,897,x+19,902),2,fill=(50,65,73,255))
txt(83,913,'−54      0 dB',9,(105,150,174,255),center=True)
txt(856,751,'MODULATION',12,(186,211,226,255),center=True)
txt(855,790,'CHORUS',19,(104,197,232,255),center=True)
txt(855,817,'3-VOICE SPREAD',11,(172,191,202,255),center=True)
d.line((793,837,917,837),fill=(42,72,89,255),width=1)
txt(793,846,'WAVE',11,(155,173,192,255),b=False)
txt(918,846,'SINE',11,(203,230,246,255),b=False)
txt(793,868,'CLOCK',11,(155,173,192,255),b=False)
txt(918,868,'FREE',11,(203,230,246,255),b=False)
d.line((793,887,917,887),fill=(42,72,89,255),width=1)
txt(855,901,'3.00 Hz',13,(206,238,247,255),center=True)
d.rounded_rectangle((476,907,715,931),6,fill=(16,28,39,255),outline=(54,137,167,255),width=1)
txt(595,916,'MANUAL  ·  3.00 Hz',12,(215,243,250,255),center=True)
bg.convert('RGB').save(r/'BreezeMod-layout-preview.jpg',quality=93)
print('Preview created')
