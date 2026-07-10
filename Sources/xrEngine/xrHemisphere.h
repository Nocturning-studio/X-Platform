#ifndef xrHemisphereH
#define xrHemisphereH

typedef void __stdcall xrHemisphereIterator(float x, float y, float z, float energy, LPVOID param);

void xrHemisphereBuild(int quality, float energy, xrHemisphereIterator* it, LPVOID param);
int xrHemisphereVertices(int quality, const fvec3*& verts);
int xrHemisphereIndices(int quality, const u16*& indices);

#endif // xrHemisphereH
