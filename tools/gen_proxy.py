# Genera los stubs de reenvío de una DLL proxy a partir de la tabla de exportación de la DLL real.
# Cada export salta a la función real, que se resuelve al cargar (proxy_init).
import sys, struct
src, name, out_s, out_def, out_h = sys.argv[1:6]
d = open(src, 'rb').read()
pe = struct.unpack_from('<I', d, 0x3c)[0]
nsec = struct.unpack_from('<H', d, pe + 6)[0]
opt = pe + 24
optsz = struct.unpack_from('<H', d, pe + 20)[0]
exp_rva = struct.unpack_from('<I', d, opt + 96)[0]
secs = [struct.unpack_from('<8sIIII', d, opt + optsz + i * 40) for i in range(nsec)]
def off(rva):
    for _, vs, va, rs, ro in secs:
        if va <= rva < va + max(vs, rs): return rva - va + ro
e = off(exp_rva)
n = struct.unpack_from('<I', d, e + 24)[0]
names_rva = struct.unpack_from('<I', d, e + 32)[0]
names = []
for i in range(n):
    o = off(struct.unpack_from('<I', d, off(names_rva) + i * 4)[0])
    names.append(d[o:d.index(b'\0', o)].decode())
with open(out_s, 'w') as f:
    f.write('# Generado por tools/gen_proxy.py\n.text\n')
    for i, nm in enumerate(names):
        f.write(f'.globl _proxy_{nm}\n_proxy_{nm}:\n    jmp *_proxy_table+{4*i}\n')
with open(out_def, 'w') as f:
    f.write(f'LIBRARY {name}\nEXPORTS\n')
    for nm in names: f.write(f'    {nm}=proxy_{nm}\n')
with open(out_h, 'w') as f:
    f.write('// Generado por tools/gen_proxy.py\n')
    f.write(f'#define PROXY_DLL "{name}"\n#define PROXY_COUNT {len(names)}\n')
    f.write('static const char *const proxy_names[PROXY_COUNT] = {\n')
    for nm in names: f.write(f'    "{nm}",\n')
    f.write('};\n')
print(len(names), 'exports')
