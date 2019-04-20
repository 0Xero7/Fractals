
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include "Complex.cpp"
#include "Color.cpp"

Color border = Color(31, 28, 24);
Color in = Color(0, 0, 0);				    //	IN SET			(DARKER)
Color out = Color(245, 239, 239);				//	NOT IN SET		(LIGHTER)

Color* LUT;

int canvas_size = 500;
int supersampling = 2;
int iteration_limit = 1000;
double zoom = 1;
double pt_x = -0.761574;
double pt_y = -0.0847596;
int it = 1;
bool it_complete = false, aa_complete = false;

Complex** a = new Complex * [canvas_size * supersampling];
int** buffer = new int* [canvas_size * supersampling];

double clamp01(double arg) { return arg > 1 ? (double)1 : arg; }

Color Lerp(Color c1, Color c2, double currentStep, double maxStep)
{
	return Color(clamp01(c1.r + (((c2.r - c1.r) / iteration_limit) * currentStep)),
		clamp01(c1.g + (((c2.g - c1.g) / iteration_limit) * currentStep)),
		clamp01(c1.b + (((c2.b - c1.b) / iteration_limit) * currentStep)));
}

void renderloop(GLFWwindow * window, int** buffer, bool refresh, int max_iterations, bool fast)
{
	if (refresh)
	{/* Render here */
		glClear(GL_COLOR_BUFFER_BIT);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, canvas_size, canvas_size, 0, 0, 1);

		glBegin(GL_POINTS);

		float shift = 0.0;
		Color shade;

		for (int i = 0; i < canvas_size * supersampling; i++)
			for (int j = 0; j < canvas_size * supersampling; j++)
			{
				if ((j / supersampling) % 1 != 0) continue;
				if (buffer[i][j] == -1)
					glColor3d(in.r, in.g, in.b);
				else
				{
					shade = LUT[buffer[i][j]];
					glColor3d(shade.r, shade.g, shade.b);
				}
				glVertex2d(i / supersampling, j / supersampling);
			}
		glEnd();
		glFinish();

		/* Swap front and back buffers */
		glfwSwapBuffers(window);
	}

	/* Poll for and process events */
	glfwPollEvents();
}

void pixel_iteration(Complex * *cache, int** buffer, double scale, double s_x, double s_y, int it_count)
{
	double delta = 5 / (canvas_size * scale * supersampling);								// GET PIXEL SIZE WRT. SCALE
	double x = s_x - (canvas_size * supersampling / 2) * delta, y = -s_y + (canvas_size * supersampling / 2) * delta;			// GET TOP-LEFT POSITION

	for (int i = 0; i < canvas_size * supersampling; i++)
	{
		for (int j = 0; j < canvas_size * supersampling; j++)
		{
			double real = (cache[i][j].real * cache[i][j].real) - (cache[i][j].im * cache[i][j].im) + (x + i * delta);
			double im = (2 * cache[i][j].im * cache[i][j].real) + (y - j * delta);

			if (real * real + im * im > 4)
				buffer[i][j] = it_count;

			cache[i][j].real = real;
			cache[i][j].im = im;
		}
	}
}

Color get(int** buffer, int x, int y, int max_iterations)
{
	if (x < 0 || x >= canvas_size * supersampling || y < 0 || y >= canvas_size * supersampling) return in;
	else if (buffer[x][y] > -1) return LUT[buffer[x][y]];
	else return in;
}

void antialias(GLFWwindow * window, bool refresh, int** buffer, int size, int max_iterations)
{
	if (refresh)
	{/* Render here */
		glClear(GL_COLOR_BUFFER_BIT);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, canvas_size, canvas_size, 0, 0, 1);

		glBegin(GL_POINTS);

		float shift = 0.0;
		Color shade;

		for (int i = 0; i < canvas_size * supersampling; i++)
		{
			for (int j = 0; j < canvas_size * supersampling; j++)
			{
				Color c;
				for (int k = -size; k <= size; k++)
				{
					for (int l = -size; l <= size; l++)
					{
						c = c + get(buffer, i + k, j + l, max_iterations);
					}
				}

				c = c / ((2 * size + 1) * (2 * size + 1));
				glColor3d(c.r, c.g, c.b);
				glVertex2d(i / supersampling, j / supersampling);
			}
		}
		glEnd();
		glFinish();

		/* Swap front and back buffers */
		glfwSwapBuffers(window);
	}

	/* Poll for and process events */
	glfwPollEvents();
}

void InitLUT()
{
	LUT = new Color[iteration_limit];
	for (int i = 0; i < iteration_limit; i++)
		LUT[i] = Lerp(out, border, i, iteration_limit);
}

void Redraw()
{
	it = 1;
	aa_complete = false;

	for (int i = 0; i < canvas_size * supersampling; i++)
	{
		for (int j = 0; j < canvas_size * supersampling; j++)
		{
			buffer[i][j] = -1;
			a[i][j].im = a[i][j].real = 0;
		}
	}
}

double l_x, l_y;
void mouse_button_callback(GLFWwindow * window, double xpos, double ypos)
{
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	{
		glfwGetCursorPos(window, &xpos, &ypos);

		if (l_x == -1)
		{
			l_x = xpos;
			l_y = ypos;
		}

		double delta_x = xpos - l_x, delta_y = ypos - l_y;

		pt_x -= delta_x / (200 * zoom);
		pt_y -= delta_y / (200 * zoom);

		l_x = xpos;
		l_y = ypos;

		Redraw();
	}
	else
	{
		l_x = l_y = -1;
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (yoffset > 0)
		zoom *= 1.2;
	else if (yoffset < 0)
		zoom /= 1.2;

	Redraw();
}

int main(void)
{
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(canvas_size, canvas_size, "Possibly, Fractals", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glHint(GL_POINT_SMOOTH, GL_NICEST);
	glHint(GL_LINE_SMOOTH, GL_NICEST);
	glHint(GL_POLYGON_SMOOTH, GL_NICEST);

	glEnable(GL_POINT_SMOOTH);
	glfwSetCursorPosCallback(window, mouse_button_callback); 
	glfwSetScrollCallback(window, scroll_callback);

	InitLUT();

	for (int i = 0; i < canvas_size * supersampling; ++i)
	{
		a[i] = new Complex[canvas_size * supersampling];
		buffer[i] = new int[canvas_size * supersampling];
	}

	for (int i = 0; i < canvas_size * supersampling; i++)
	{
		for (int j = 0; j < canvas_size * supersampling; j++)
		{
			buffer[i][j] = -1;
		}
	}

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		it_complete = !(it <= iteration_limit);

		if (!it_complete)
		{
			pixel_iteration(a, buffer, zoom, pt_x, pt_y, it++);
			if (it % 50 == 0)
				std::cout << "Iteration " << it << std::endl;

			if (it > 2 && ((it < 10 && it % 2 == 0) || (it < 50 && it % 20 == 0) || (it < 100 && it % 25 == 0) || (it > 100 && it % 100 == 0)))
				renderloop(window, buffer, 1, iteration_limit, true);
			else
				glfwPollEvents();
		}
		else                // Iterations done
		{
			if (!aa_complete)
			{
				std::cout << "\nAntialiasing started." << std::endl;
				antialias(window, 1, buffer, 1, iteration_limit);
				aa_complete = true;
				std::cout << "\nAntialiasing complete." << std::endl;
			}
			glfwPollEvents();
		}
	}

	for (int i = 0; i < canvas_size * supersampling; ++i)
	{
		delete(a[i]);
		delete(buffer[i]);
	}

	delete(a);
	delete(buffer);

	glfwTerminate();
	return 0;
}