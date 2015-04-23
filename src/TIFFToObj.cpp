/*
 ============================================================================
 Name        : TIFFToObj.cpp
 Author      : David O'Mahony
 Version     :
 Copyright   : GPL
 Description : Hello World in C++,
 ============================================================================
 */

#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include "xtiffio.h"
#include "tiffio.h"
#include "geo_normalize.h"
#include <glm.hpp>

using namespace std;
using namespace glm;

struct tri {
	int v[3];
};

static void write_tile(int tx, int ty, TIFF *tif);

int main(int argc, char **argv) {

	TIFF *tif = 0;
	cout << "Opening File " << argv[1] << flush;
	tif = XTIFFOpen(argv[1], "r");
	cout << " Done" << endl;

	GTIF* gtif = GTIFNew(tif);
	GTIFDefn def;
	GTIFGetDefn(gtif, &def);

	double x = 0;
	double y = 0;
	GTIFImageToPCS(gtif, &x, &y);

	for (int tx = 0; tx < 16; ++tx) {
		for (int ty = 0; ty < 16; ++ty) {
			write_tile(tx, ty, tif);
		}
	}
}

void write_tile(int tx, int ty, TIFF *tif)
{
 	GTIF* gtif = GTIFNew(tif);

	auto nstrips = TIFFNumberOfStrips(tif);
    auto strip_size = TIFFStripSize(tif);

    nstrips = nstrips / 16;
    strip_size = strip_size / 16;

    tstrip_t strip = 0;

    int x_off = (strip_size / sizeof(short)) * tx;
    int y_off = nstrips * ty;

    cout << "N Strips: " << nstrips << " x " << strip_size << endl;

    short* buf= static_cast<short*>(_TIFFmalloc(TIFFScanlineSize(tif)));

	vector<vec3> vertices;
	vector<vec3> normals;
	vector<tri> triangles;
	vector<vector<int>> vert_to_triangles;

	cout << "Reading Tile " << tx << "," << ty << flush;
    for (int y = 0; y < nstrips; y++) {
    	TIFFReadEncodedStrip(tif, y + y_off, buf, -1);
    	for (int x = 0; x < strip_size / sizeof(short); x++) {
    		vec3 v;
     		v.x = (x + x_off) / 10.0;
    		v.z = (TIFFNumberOfStrips(tif) - (y + y_off)) / 10.0;
    		v.y = buf[x + x_off] / 100.0;
    		vertices.push_back(v);
    		vector<int> vn;
    		vert_to_triangles.push_back(vn);
    	}
    }
	cout << " Done" << endl;

	string fname;

	fname += to_string(tx) + "_" + to_string(ty) + ".obj";
    ofstream ofs;
    ofs.open(fname);

	cout << "Writing Verts" << flush;
    for (auto V = vertices.begin(); V != vertices.end(); ++V) {
    	ofs << "v " << V->x << " " << V->y << " " << V->z << endl;
    }
	cout << " Done" << endl;

	cout << "Reading Triangle Verts" << flush;
    for (int y = 0; y < nstrips -1; y++) {
    	for (int x = 0; x < strip_size / sizeof(short) -1; x++) {
    		tri t;
    		t.v[0] = y * nstrips + x;
    		t.v[1] = (y + 1) * nstrips + x;
    		t.v[2] = y * strip_size / sizeof(short) + (x + 1);
    		triangles.push_back(t);

    		vert_to_triangles[t.v[0]].push_back(triangles.size() - 1);
    		vert_to_triangles[t.v[1]].push_back(triangles.size() - 1);
    		vert_to_triangles[t.v[2]].push_back(triangles.size() - 1);

    		tri t2;
     		t2.v[0] = t.v[2];
     		t2.v[1] = t.v[1];
    		t2.v[2] = (y + 1) * nstrips + (x + 1);
    		triangles.push_back(t2);

      		vert_to_triangles[t2.v[0]].push_back(triangles.size() - 1);
    		vert_to_triangles[t2.v[1]].push_back(triangles.size() - 1);
    		vert_to_triangles[t2.v[2]].push_back(triangles.size() - 1);
    	}
    }
	cout << " Done" << endl;

	cout << "Calculating Normals" << flush;
    for (auto V = vert_to_triangles.begin(); V != vert_to_triangles.end(); ++V) {
    	vec3 n;
    	vec3 n1(-1,-1,-1);
    	for (auto VV = V->begin(); VV != V->end(); ++VV) {

    		tri t = triangles[*VV];
    		n +=  (n1 * cross(vertices[t.v[0]] - vertices[t.v[1]], vertices[t.v[0]] - vertices[t.v[2]]));

    	}
    	normals.push_back(normalize(n));
    }
	cout << " Done" << endl;

	cout << "Writing Normals" << flush;
    for (auto V = normals.begin(); V != normals.end(); ++V) {
    	ofs << "vn " << V->x << " " << V->y << " " << V->z << endl;
    }
	cout << " Done" << endl;

	cout << "Writing Faces" << flush;
    for (auto V = triangles.begin(); V != triangles.end(); ++V) {
    	ofs << "f " << V->v[0] + 1 << "//" << V->v[0] + 1 << " ";
    	ofs << V->v[1] + 1 << "//" << V->v[1] + 1<< " ";
    	ofs << V->v[2] + 1 << "//" << V->v[2] + 1<< endl;
    }
	cout << " Done" << endl;

    ofs.close();

    /*
    for (int i = 0; i < TIFFStripSize(tif); i += sizeof(short)) {
    	std::cout << i << ": " << static_cast<short>(buf[i]) << std::endl;
    }
    */

    if (0 && tif) {
        tdata_t buf;
        tstrip_t strip;

        buf = _TIFFmalloc(TIFFStripSize(tif));
        for (strip = 0; strip < TIFFNumberOfStrips(tif); strip++)
            TIFFReadEncodedStrip(tif, strip, buf, (tsize_t) -1);
        _TIFFfree(buf);
        TIFFClose(tif);
    }

	int w, h;

    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
}
