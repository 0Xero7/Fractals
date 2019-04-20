
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include "Complex.cpp"
#include "Color.cpp"

Color c1 = Color(255, 255, 255);
Color c2 = Color(20, 20, 20);

int canvas_size = 1000;
int supersampling = 2;

double clamp01(double arg) { return arg > 1 ? (double)1 : arg; }

Color Lerp(Color c1, Color c2, double currentStep, double maxStep)
{
	return Color(clamp01(c1.r + ((c2.r - c1.r) / maxStep * currentStep)),
		clamp01(c1.g + ((c2.g - c1.g) / maxStep * currentStep)),
		clamp01(c1.b + ((c2.b - c1.b) / maxStep * currentStep)));
}

void renderloop(GLFWwindow * window, int** buffer, bool refresh, int max_iterations)
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
				if (buffer[i][j] == -1)
					glColor3d(0, 0, 0);
				else
				{
					shade = Lerp(c2, c1, buffer[i][j], max_iterations);
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
	if (x < 0 || x >= canvas_size * supersampling || y < 0 || y >= canvas_size * supersampling) return Color(0,0,0);
	else return Lerp(c2, c1, buffer[x][y], max_iterations);
}

void antialias(GLFWwindow *window, bool refresh, int** buffer, int size, int max_iterations)
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

	Complex** a = new Complex * [canvas_size * supersampling];
	int** buffer = new int* [canvas_size * supersampling];

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

	int it = 1;

	int iteration_limit = 1000;
	const double zoom = 71256;

	bool it_complete = false, aa_complete = false;
	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		it_complete = !(it <= iteration_limit);

		if (!it_complete)
		{
			pixel_iteration(a, buffer, zoom, -0.761574, -0.0847596, it++);
			if (it % 50 == 0)
				std::cout << "Iteration " << it << std::endl;

			renderloop(window, buffer, (it % 100 == 0), iteration_limit);
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