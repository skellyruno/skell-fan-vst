"""Deterministic, original transparent artwork; reference picture used only for base.
Run: python scripts/generate_assets.py
No external design assets, brands or nonredistributable fonts bundled.
"""
from pathlib import Path
import math
import numpy as np
from PIL import Image, ImageDraw, ImageFilter
import cv2

OUT=Path(__file__).resolve().parents[1]/'Assets'
S=3

def new(w,h): return Image.new('RGBA',(w*S,h*S),(0,0,0,0))
def small(img):return img.resize((img.width//S,img.height//S),Image.Resampling.LANCZOS)
def save(img,name):small(img).save(OUT/name)
def xy(points):return [(int(x*S),int(y*S)) for x,y in points]
def ellipse(d,box,**kw):d.ellipse(tuple(round(v*S) for v in box),**kw)
def line(d,p,fill,width):d.line(xy(p), fill=fill, width=int(width*S), joint='curve')

# Fan body: actual user-provided photo as texture for the body alone, with the
# spinning fan head and control plate removed from the source layer.
source=np.array(Image.open(OUT/'reference-fan.jpg').convert('RGB'))
# erase original attribution on the plinth; BreezeMod is vector-painted.
mask=np.zeros(source.shape[:2],np.uint8)
cv2.rectangle(mask,(411,938),(555,980),255,-1)
source=cv2.inpaint(source,mask,12,cv2.INPAINT_TELEA)
alpha=np.zeros(source.shape[:2],np.uint8)
bodypoly=np.array([(246,614),(205,664),(170,795),(154,872),(156,929),(175,951),
                   (219,969),(253,982),(709,983),(750,971),(779,945),(785,917),
                   (772,831),(744,681),(718,623),(687,603),(651,600),(274,603)],np.int32)
cv2.fillPoly(alpha,[bodypoly],255)
alpha=cv2.GaussianBlur(alpha,(5,5),1.5)
# Remove partial original fan/cage from the cutout's upper edge. Stand and blade
# boundary are covered by the new rotating fan layers.
source=np.dstack((source,alpha))
Image.fromarray(source,'RGBA').save(OUT/'fan_body.png')

# Blade disc: 3 curved wide blue fan blades, metallic texture/shading highlights.
w=600; img=new(w,w); d=ImageDraw.Draw(img)
cx=cy=300*S
for k in range(3):
    a=k*2*math.pi/3-0.48
    def pt(radius, theta):return (300+radius*math.cos(theta),300+radius*math.sin(theta))
    p=[pt(70,a-0.08),pt(145,a-0.34),pt(252,a-0.65),pt(272,a-0.45),
       pt(270,a+0.04),pt(245,a+0.42),pt(194,a+0.61),pt(120,a+0.45),pt(67,a+0.26)]
    d.polygon(xy(p), fill=(31,161,217,229))
    line(d,[p[1],p[2],p[3]],(166,230,255,135),3)
    for j in range(4):
        r=138+j*28
        line(d,[pt(r,a-0.25),pt(r+28,a+0.25)],(156,232,255,29),3)
    line(d,[p[-1],p[-2],p[-3]],(9,69,118,101),6)
ellipse(d,(247,247,353,353), fill=(69,114,145,240), outline=(178,211,221,250),width=8*S)
save(img,'blades.png')
img=img.filter(ImageFilter.GaussianBlur(22*S)); save(img,'blades_blur.png')

# Grille, four feet/clips, concentric steel front loops and 88 radial wires.
g=new(640,640);d=ImageDraw.Draw(g)
for r,wid,col in [(306,7,(58,61,67,235)),(302,3,(231,239,245,245)),(246,5,(69,76,81,250)),
                  (242,2,(218,228,234,235)),(95,5,(50,54,58,254)),(90,3,(209,213,217,250))]:
    ellipse(d,(320-r,320-r,320+r,320+r),outline=col,width=wid*S)
for a in np.linspace(0,math.pi*2,88,endpoint=False):
    sn=math.sin(a);cs=math.cos(a)
    start=(320+92*cs,320+92*sn); end=(320+301*cs,320+301*sn)
    line(d,[start,end],(29,35,40,180),2.5)
    line(d,[(start[0]-0.9,start[1]),(end[0]-0.9,end[1])],(211,216,221,215),1.2)
for a in [0.32,1.88,3.45,5.0]:
    p1=(320+291*math.cos(a),320+291*math.sin(a));p2=(320+319*math.cos(a),320+319*math.sin(a))
    line(d,[p1,p2],(135,137,141,255),9)
    line(d,[p1,p2],(223,227,234,180),3)
save(g,'grille.png')

h=new(190,190);d=ImageDraw.Draw(h)
for rad,col in [(92,(51,56,60,255)),(88,(164,176,186,255)),(80,(33,36,42,255)),
                (68,(38,124,164,255)),(56,(48,150,190,255)),(43,(68,72,77,255)),
                (31,(181,187,188,255)),(21,(86,95,102,255))]:
    ellipse(d,(95-rad,95-rad,95+rad,95+rad),fill=col)
for j in range(30):
    a=j*2*math.pi/30
    ellipse(d,(95+84*math.cos(a)-2,95+84*math.sin(a)-2,95+84*math.cos(a)+2,95+84*math.sin(a)+2),fill=(210,216,215,210))
ellipse(d,(74,63,106,83),fill=(222,230,234,110))
save(h,'hub_cap.png')

for color, name, size in [((135,145,153),'knob_chrome.png',80),((66,165,211),'knob_blue.png',42)]:
    im=new(size,size);d=ImageDraw.Draw(im);m=size/2
    for r,c in [(m-2,(38,39,41,245)),(m-6,(216,223,228,255)),(m-11,(66,70,76,255)),
                (m-14,(*color,255)),(m-18,(91,99,106,255))]:
        ellipse(d,(m-r,m-r,m+r,m+r),fill=c)
    line(d,[(m,m-5),(m-8,m-25)],(232,76,75,255),4)
    save(im,name)
for name,sz in [('key_strip.png',(110,148)),('small_key.png',(72,48))]:
    im=new(*sz);d=ImageDraw.Draw(im);w,h=sz
    ellipse(d,(1,2,w-2,h/2-2),fill=(229,231,232,255))
    ellipse(d,(1,h/2+2,w-2,h-2),fill=(34,181,231,255))
    save(im,name)
im=new(22,40);d=ImageDraw.Draw(im)
d.rounded_rectangle((3*S,2*S,19*S,36*S),radius=5*S,fill=(192,200,207,255),outline=(73,78,82,255),width=3*S)
save(im,'slider_thumb.png')
print('Generated layered PNGs in',OUT)
