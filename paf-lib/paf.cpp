#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <limits>
#include <vector>
#include <map>

constexpr uint32_t WIDTH = 128;
constexpr uint32_t HEIGHT = 128;


struct point_t {
	int32_t x, y;

	point_t& operator+=(const point_t& rhs) {
		this->x += rhs.x; this->y += rhs.y; return *this;
	}
	friend point_t operator+(point_t lhs, const point_t& rhs) {
		lhs += rhs; return lhs;
	}

	point_t& operator-=(const point_t& rhs) {
		this->x -= rhs.x; this->y -= rhs.y; return *this;
	}
	friend point_t operator-(point_t lhs, const point_t& rhs) {
		lhs -= rhs; return lhs;
	}

};

struct rect_t {
	point_t tl, br;
};

struct contour_t {
	std::vector <point_t> points;
};

struct character_t {
	uint32_t code;
	std::vector <contour_t> contours;

	rect_t bounds() const {
		rect_t b = {
			{std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max()}, 
			{std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min()}
		};
		for(auto &contour : this->contours) {
			for(auto &p : contour.points) {
				b.tl.x = p.x < b.tl.x ? p.x : b.tl.x;
				b.tl.y = p.y < b.tl.y ? p.y : b.tl.y;
				b.br.x = p.x > b.br.x ? p.x : b.br.x;
				b.br.y = p.y > b.br.y ? p.y : b.br.y;
			}
		}
		return b;
	}
};

std::map <uint32_t, character_t> codes;


/* 
	defines the callback function for rendering spans. will be called for each
	span or single pixel that needs drawing to render the character.

	x, y, len are the coordinates and length of the span in polygon space,
	and v is the value from 0..255 to either draw or blend with.
*/
typedef void (*span_callback_t)(int32_t x, int32_t y, uint32_t len, uint8_t v);
span_callback_t span_callback = nullptr;
void set_span_callback(span_callback_t cb) {
	span_callback = cb;
}

/*
	set the level of oversampling required
*/
enum class oversample {x1, x4, x16};
oversample sampling = oversample::x1;
void set_oversampling(oversample s = oversample::x1) {
	sampling = s;
}

/*
	define the rectangle to clip drawing to (usually this would be the size of 
	the target screen, or the current clip rectangle defined in your graphics api)
*/
rect_t clip;
void set_clip(rect_t &r) {
	clip = r;
}

constexpr uint32_t SSB_SIZE = 128;
static uint32_t ssb[SSB_SIZE / 32][SSB_SIZE]; // 2kB 128x128 1-bit buffer for super sampling
void render_character(const character_t &character, uint32_t size) {
	memset(ssb, 0, sizeof(ssb));

	rect_t bounds = character.bounds();

	// vertically traverse contour lines and xor into super sampling buffer
	for(auto &contour : character.contours) {
		for(auto i = 0; i < contour.points.size(); i++) {
			// get start and end points of segment (last sgement wraps back to first point to close loop)
			const point_t &s = contour.points[i] - bounds.tl;
			const point_t &e = (i + 1 < contour.points.size() ? contour.points[i + 1] : contour.points[0]) - bounds.tl;

			// current x value and 16:16fp step x value
			int32_t cx = 0;
			int32_t dx = ((e.x - s.x) << 16) / abs(e.y - s.y);

			// step y up or down
			int32_t cy = s.y;
			int32_t dy = e.y < s.y ? -1 : 1;
			int32_t count = abs(e.y - s.y);

			// loop through each scanline
			while(count--) {
				// get current x coordinate
				int32_t x = (cx >> 16) + s.x;

				// clamp x value to buffer
				x = x < 0 ? 0 : x;
				x = x >= clip.br.x ? clip.br.x - 1 : x;

				uint32_t ssb_x = x / 32;
				uint32_t ssb_bit_mask = 0x80000000 >> (x & 0b11111);

				ssb[ssb_x][cy] ^= ssb_bit_mask;

				cx += dx;
				cy += dy;
			}
		}
	}

	// fill spans between "on" pixels in super sampling buffer
/*	for(auto y = 0; y < SSB_SIZE; y++) {
		auto x = 0;

		uint8_t v = ssb[0][y] & 0x80000000;
		while(x < SSB_SIZE) {

			uint32_t ssb_x = x / 32;
			uint32_t ssb_bit_mask = 0x80000000 >> (x & 0b11111);

			x++;
		}
	}*/
}

contour_t g_outer = {
	.points = {{0,-64}, {-3,-62}, {-5,-59}, {-6,-55}, {-7,-50}, {-6,-45}, {-5,-41}, {-4,-37}, {-3,-33}, {-1,-29}, {0,-24}, {2,-21}, {6,-17}, {8,-14}, {12,-12}, {15,-8}, {19,-5}, {22,-1}, {26,1}, {30,5}, {30,5}, {30,8}, {30,10}, {30,11}, {31,14}, {29,12}, {28,11}, {26,10}, {25,9}, {23,9}, {19,7}, {17,6}, {15,5}, {13,5}, {11,5}, {8,5}, {5,6}, {3,9}, {3,12}, {3,15}, {3,18}, {4,22}, {7,27}, {10,32}, {14,38}, {18,42}, {22,46}, {26,48}, {30,50}, {35,51}, {35,50}, {36,50}, {37,49}, {37,48}, {38,48}, {37,46}, {37,45}, {36,45}, {35,45}, {35,45}, {31,44}, {27,43}, {25,42}, {21,40}, {19,38}, {16,34}, {14,31}, {12,28}, {10,24}, {9,21}, {8,19}, {8,19}, {8,17}, {8,17}, {8,17}, {8,14}, {8,12}, {9,11}, {11,11}, {13,11}, {13,11}, {13,11}, {14,11}, {14,11}, {16,12}, {21,14}, {26,18}, {30,23}, {33,29}, {36,38}, {36,38}, {36,38}, {36,39}, {37,39}, {39,40}, {39,39}, {39,39}, {40,38}, {40,38}, {41,38}, {40,37}, {40,37}, {40,37}, {40,37}, {40,37}, {37,31}, {37,26}, {37,20}, {37,15}, {37,14}, {37,12}, {37,12}, {37,12}, {37,12}, {39,12}, {39,12}, {39,12}, {39,12}, {39,12}, {40,13}, {40,13}, {41,13}, {42,14}, {43,14}, {45,16}, {45,16}, {45,16}, {46,16}, {46,16}, {48,17}, {48,16}, {48,16}, {48,15}, {48,15}, {49,15}, {48,13}, {48,13}, {48,13}, {48,13}, {48,13}, {45,10}, {42,9}, {40,7}, {37,5}, {36,5}, {33,0}, {33,-5}, {33,-10}, {33,-15}, {33,-20}, {31,-26}, {29,-32}, {28,-38}, {26,-43}, {24,-49}, {22,-52}, {20,-55}, {18,-58}, {16,-61}, {15,-65}, {12,-65}, {10,-65}, {7,-65}, {5,-65}, {3,-65}}
};

contour_t g_inner = {
	.points = {{8,-57}, {15,-51}, {21,-40}, {25,-26}, {29,-8}, {29,-6}, {29,-6}, {29,-6}, {29,-6}, {29,-6}, {28,-4}, {28,-3}, {28,-3}, {28,-3}, {28,-3}, {26,-3}, {26,-3}, {24,-3}, {24,-3}, {24,-5}, {21,-5}, {20,-6}, {19,-8}, {16,-9}, {16,-12}, {12,-14}, {9,-18}, {6,-22}, {4,-26}, {2,-30}, {0,-34}, {-1,-38}, {-2,-42}, {-3,-46}, {-4,-50}, {-4,-52}, {-4,-54}, {-4,-56}, {-4,-58}, {-4,-60}, {-3,-60}, {-2,-60}, {-1,-60}, {0,-60}, {1,-60}}
};

character_t g = {
	.code = 103,
	.contours = {g_outer, g_inner}
};

uint8_t buf[WIDTH][HEIGHT];
void span(int32_t x, int32_t y, uint32_t len, uint8_t v) {
	uint8_t *p = &buf[x][y];
	while(len--) {*p = v;}
}

int main() {

	rect_t clip = {{0, 0}, {WIDTH, HEIGHT}};
	set_clip(clip);
	set_span_callback(span);
	render_character(g, 12);

	for(int y = 0; y < HEIGHT; y++) {
		for(int x = 0; x < HEIGHT; x++) {
			uint32_t ssb_x = x / 32;
			uint32_t bit = 0x80000000 >> (x & 0b11111);
			buf[HEIGHT - y][x] = ssb[ssb_x][y] & bit ? 255 : 0;
		}
	}

  stbi_write_png("out.png", 128, 128, 1, (void *)buf, 128);

	return 0;
}
