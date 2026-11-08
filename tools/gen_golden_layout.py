import sys, struct, random, glob
sys.path.insert(0,'/home/claude/fs12/rebuild/tools')
from oracle import Oracle
o=Oracle(glob.glob('/home/claude/fs12/apk/lib/*/libapp.so')[0])
S=lambda n:'GenericGUIManager::'+n
this=o.alloc(0x6000); dev=o.alloc(0x80000)
fb=lambda f: struct.unpack('<I',struct.pack('<f',f))[0]
bf=lambda u: struct.unpack('<f',struct.pack('<I',u))[0]
random.seed(7)
lines=[]
# updateScreenScale
for aspect in (0.75,1.0,1.3333,1.5,1.6,1.7777778,2.0,2.1666667,2.4):
    o.wf(dev+4,aspect); o.wf(dev+0xc,1.0)
    o.call(S('updateScreenScale(GLESHandheldRenderDevice*)'), this, dev)  # (this, dev) cagri duzeni: r0=this r1=dev
    lines.append('S %.9g %.9g %.9g'%(aspect,o.rdf(this+0x5c6c),o.rdf(this+0x5c70)))
# align / undo / anchor
o.wf(dev+4,1.7777778); o.call(S('updateScreenScale(GLESHandheldRenderDevice*)'), this, dev)
sx,sy=o.rdf(this+0x5c6c),o.rdf(this+0x5c70)
buf=o.alloc(64)
for i in range(300):
    x,y,w,h=[bf(fb(random.uniform(-0.2,1.2))) for _ in range(4)]
    a=random.choice([0,1,4,8,0x10,0x14,0x40,0x80,0x41,0x5,0x90,0x44,0x48,0x11,0x50])
    for k,v in enumerate((x,y,w,h)): o.wf(buf+4*k,v)
    o.call(S('applyScreenAlign(float&, float&, float&, float&, unsigned int)'), this, buf, buf+4, buf+8, buf+12, a) if False else None
    # 5 arguman: this,r1,r2,r3 + stack
    o._call_raw(o.sym[S('applyScreenAlign(float&, float&, float&, float&, unsigned int)')], [this,buf,buf+4,buf+8,buf+12,a])
    r=[o.rdf(buf+4*k) for k in range(4)]
    lines.append('A %.9g %.9g %.9g %.9g %d %.9g %.9g %.9g %.9g'%(x,y,w,h,a,*r))
    for k,v in enumerate((x,y)): o.wf(buf+4*k,v)
    o._call_raw(o.sym[S('undoScreenAlignPosition(float&, float&, unsigned int)')], [this,buf,buf+4,a])
    lines.append('U %.9g %.9g %d %.9g %.9g'%(x,y,a,o.rdf(buf),o.rdf(buf+4)))
    for k,v in enumerate((x,y,w,h)): o.wf(buf+4*k,v)
    o._call_raw(o.sym['FUN_0003e960'] if 'FUN_0003e960' in o.sym else 0x2e960|1, [buf,buf+4,fb(w),fb(h),a])
    lines.append('P %.9g %.9g %.9g %.9g %d %.9g %.9g'%(x,y,w,h,a,o.rdf(buf),o.rdf(buf+4)))
open('/home/claude/fs12/rebuild/tests/golden/gui_layout.txt','w').write('\n'.join(lines)+'\n')
print(len(lines), lines[:3])
