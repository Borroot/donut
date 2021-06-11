#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#define CSI "\x1b["  /* Control Sequencer Introducer */

#define PADDING 0.2  /* stay 10% away from terminal border */
#define D2 10  /* distance from viewer till z=0 */

#define R1 1  /* radius of the circle */
#define R2 2  /* distance from torus center to circle center */

/* light vector, must have a magnitude of 1 */
#define Lx  0
#define Ly  0.7071
#define Lz -0.7071

#define C_STEP ((2 * M_PI) / 500)  /* step size for circle */
#define T_STEP ((2 * M_PI) / 500)  /* step size for torus */
#define X_STEP ((2 * M_PI) / 150)  /* step size for x rotation */
#define Z_STEP ((2 * M_PI) / 250)  /* step size for z rotation */

float *zbuffer;
char *pixels;

void interrupt(int signal)
{
	printf(CSI "?25h");  /* make cursor visible */
	free(zbuffer);
	free(pixels);
	exit(EXIT_SUCCESS);
}

void draw(char *pixels, size_t rows, size_t cols)
{
	printf(CSI "2J");    /* clear screen */
	printf(CSI "1;1H");  /* place cursor at top left */

	for (size_t y = 0; y < rows; y++)
		for (size_t x = 0; x < cols; x++)
			printf("%c", pixels[cols * y + x]);

	fflush(stdout);
}

int main(void)
{
	signal(SIGINT, interrupt);
	printf(CSI "?25l");  /* make cursor invisible */

	struct winsize size;
	ioctl(STDIN_FILENO, TIOCGWINSZ, &size);  /* get the terminal size */

	const size_t rows = size.ws_row;  /* number of rows on terminal */
	const size_t cols = size.ws_col;  /* number of cols on terminal */

	zbuffer = malloc(sizeof(*zbuffer) * rows * cols);
	pixels  = malloc(sizeof(*pixels)  * rows * cols);

	const float minsize = rows < cols ? rows : cols;  /* smallest size */
	const float maxterm = (minsize / 2) * (1 - PADDING);  /* max pixel on term */
	const float D1 = (D2 * maxterm) / (R1 + R2);  /* distance from viewer till screen */

	float x_rot = 0;  /* x rotation angle */
	float z_rot = 0;  /* z rotation angle */

	while (1) {
		memset(zbuffer,  0, sizeof(*zbuffer) * rows * cols);  /* initialize to 0's */
		memset(pixels, ' ', rows * cols);  /* initialize to spaces */

		x_rot = x_rot < 2 * M_PI ? x_rot + X_STEP : x_rot + X_STEP - 2 * M_PI;
		z_rot = z_rot < 2 * M_PI ? z_rot + Z_STEP : z_rot + Z_STEP - 2 * M_PI;

		for (float c_angle = 0; c_angle < 2 * M_PI; c_angle += C_STEP) {
			for (float t_angle = 0; t_angle < 2 * M_PI; t_angle += T_STEP) {
				/* precompute a lot of expressions */
				float cos_c = cosf(c_angle), cos_t = cosf(t_angle), cos_x = cosf(x_rot), cos_z = cosf(z_rot);
				float sin_c = sinf(c_angle), sin_t = sinf(t_angle), sin_x = sinf(x_rot), sin_z = sinf(z_rot);
				float subexpr = R2 + R1 * cos_c;

				/* calculate the position of the point */
				float x = subexpr * (cos_t * cos_z + sin_t * sin_x * sin_z) - R1 * sin_c * cos_x * sin_z;
				float y = subexpr * (cos_t * sin_z - sin_t * sin_x * cos_z) + R1 * sin_c * cos_x * cos_z;
				float z = subexpr * sin_t * cos_x + R1 * sin_x * sin_c;

				/* calculate the projection point, -y because coordinates are inverse in the terminal and
				 * 2 * x because two adjacent cells make up a square cell, otherwise the torus is squashed */
				float divisor = 1 / (D2 + z);
				int xproj = floorf(D1 * 2 * x * divisor + cols / 2);
				int yproj = floorf(D1 *    -y * divisor + rows / 2);

				if (0 <= yproj && yproj < (int)rows && 0 <= xproj && xproj < (int)cols
				   && divisor > zbuffer[cols * yproj + xproj]) {
					zbuffer[cols * yproj + xproj] = divisor;  /* update the zbuffer */

					/* calculate the normal vector */
					float nx = cos_c * cos_t * cos_z - sin_z * (sin_c * cos_x - cos_c * sin_t * sin_x);
					float ny = cos_c * cos_t * sin_z + cos_z * (sin_c * cos_x - cos_c * sin_t * sin_x);
					float nz = cos_c * sin_t * cos_x + sin_c * sin_x;

					/* calculate the dot product of the normal vector and light vector */
					float cos_angle = nx * Lx + ny * Ly + nz * Lz;
					char symbols[] = ".,-~:;=!*#$@";
					pixels[cols * yproj + xproj] = symbols[(int)((cos_angle + 1) * 6)];
				}
			}
		}
		draw(pixels, rows, cols);  /* draw the pixels on the screen */

		/* wait a moment with drawing the next frame */
		struct timespec ts;
		ts.tv_nsec = 10000000;
		nanosleep(&ts, &ts);
	}
}
