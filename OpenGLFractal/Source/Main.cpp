
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include "Complex.cpp"
#include "Color.cpp"
#include <thread>
#include <chrono>

Color border = Color("#000000");
Color in     = Color("#ffffff");				    //	IN SET			(DARKER)
Color out    = Color("#00dbde");//Color(245, 239, 239);				//	NOT IN SET		(LIGHTER)

Color* LUT;

int canvas_size = 800;
int supersampling = 2;
int iteration_limit = 1000;
double zoom = 1;
double pt_x = -0.761574;
double pt_y = -0.0847596;
int it = 1;
bool it_complete = false, aa_complete = false;

int gradient_breaks = 40;

int REDRAW_FLAG = 0,THREADS_RDY = 1;

int threads = 4;

Complex** a = new Complex * [canvas_size * supersampling];
int** buffer = new int* [canvas_size * supersampling];

double clamp01(double arg) { return arg > 1 ? (double)1 : arg; }

Color Lerp(Color c1, Color c2, double currentStep)
{
	return Color(clamp01(c1.r + (((c2.r - c1.r) / gradient_breaks) * currentStep)),
		clamp01(c1.g + (((c2.g - c1.g) / gradient_breaks) * currentStep)),
		clamp01(c1.b + (((c2.b - c1.b) / gradient_breaks) * currentStep)));
}
std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

void threaded_render(Complex** cache, int** buffer, int it_count, int s_x, int s_y, int e_x, int e_y, int thrd_count)
{
	double delta = 5 / (canvas_size * zoom * supersampling);																	// GET PIXEL SIZE WRT. SCALE
	double x = pt_x - (canvas_size * supersampling / 2) * delta, y = -pt_y + (canvas_size * supersampling / 2) * delta;			// GET TOP-LEFT POSITION
	double r, img, real, im;

	if (!REDRAW_FLAG)
	{
		for (int i = s_x; i < e_x; i++)
		{
			for (int j = s_y; j < e_y; j++)
			{
				if (buffer[i][j] > -1) continue;
				r = cache[i][j].real;
				img = cache[i][j].im;

				real = (r * r) - (img * img) + (x + i * delta);
				im = (r * img);
				im += im + (y - j * delta);

				if (real * real + im * im > 4)
					buffer[i][j] = it_count;

				cache[i][j].real = real;
				cache[i][j].im = im;
			}
		}
	}
	else
	{
		REDRAW_FLAG &= ~(1 << (thrd_count-1));
		for (int i = s_x; i < e_x; i++)
		{
			for (int j = s_y; j < e_y; j++)
			{
				real =  (x + i * delta);
				im = (y - j * delta);

				if (real * real + im * im > 4)
					buffer[i][j] = it_count;
				else
					buffer[i][j] = -1;

				cache[i][j].real = real;
				cache[i][j].im = im;
			}
		}
	}
}

void threaded_render_iterlaced(Complex** cache, int** buffer, int it_count, int offset)
{
	double delta = 5 / (canvas_size * zoom * supersampling);																	// GET PIXEL SIZE WRT. SCALE
	double x = pt_x - (canvas_size * supersampling / 2) * delta, y = -pt_y + (canvas_size * supersampling / 2) * delta;			// GET TOP-LEFT POSITION

	int pixels = canvas_size * supersampling * canvas_size * supersampling;
	int i = offset, j = 0;

	for (int q = offset; q < pixels; q+= threads)
	{
		double real = (cache[i][j].real * cache[i][j].real) - (cache[i][j].im * cache[i][j].im) + (x + i * delta);
		double im = (2 * cache[i][j].im * cache[i][j].real) + (y - j * delta);

		if (real * real + im * im > 4)
			buffer[i][j] = it_count;

		cache[i][j].real = real;
		cache[i][j].im = im;

		i += threads;
		if (i >= canvas_size * supersampling)
		{
			i %= canvas_size * supersampling;
			j++;
		}
	}
}

void renderloop(GLFWwindow * window, int** buffer, bool refresh, int downsample)
{
	if (refresh)
	{/* Render here */
		glClear(GL_COLOR_BUFFER_BIT);
		glBegin(GL_POINTS);

		Color shade;

		int totalpixels = canvas_size * supersampling;
		double pixel_x = 0, pixel_y = 0, pixel_delta = downsample, loop_delta = downsample * supersampling;

		for (int i = 0; i < totalpixels; i+= loop_delta)
		{
			for (int j = 0; j < totalpixels; j+= loop_delta)
			{
				if (buffer[i][j] == -1)
					glColor3d(in.r, in.g, in.b);
				else
				{
					shade = LUT[(buffer[i][j] % gradient_breaks)];
					glColor3d(shade.r, shade.g, shade.b);
				}
				glVertex2d(pixel_x, pixel_y);
				pixel_y += pixel_delta;
			}
			pixel_y = 0;
			pixel_x += pixel_delta;
		}
		glEnd();
		glFinish();

		/* Swap front and back buffers */
		glfwSwapBuffers(window);
	}

	/* Poll for and process events */
	glfwPollEvents();
}

void pixel_iteration(Complex **cache, int** buffer, double scale, double s_x, double s_y, int it_count)
{
	double delta = 5 / (canvas_size * scale * supersampling);																	// GET PIXEL SIZE WRT. SCALE
	double x = s_x - (canvas_size * supersampling / 2) * delta, y = -s_y + (canvas_size * supersampling / 2) * delta;			// GET TOP-LEFT POSITION

	int _d = canvas_size * supersampling / threads;

	std::thread t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16;

	THREADS_RDY = 0;
	switch (threads)
	{
	case 1:
		t1 = std::thread{ threaded_render, cache, buffer, it_count, 0,      0, canvas_size * supersampling ,       _d, 1 };
		t1.join();																		   
		break;																			   
	case 2:																				   
		t1 = std::thread{ threaded_render, cache, buffer, it_count, 0,      0, canvas_size * supersampling ,       _d, 1 };
		t2 = std::thread{ threaded_render, cache, buffer, it_count, 0,     _d, canvas_size * supersampling , (2 * _d), 2 };
		t1.join();																		   
		t2.join();																		   
		break;																			   
	case 4:																				   
		t1 = std::thread{ threaded_render, cache, buffer, it_count, 0,      0, canvas_size * supersampling,      _d,  1 };
		t2 = std::thread{ threaded_render, cache, buffer, it_count, 0,     _d, canvas_size * supersampling, (2 * _d), 2 };
		t3 = std::thread{ threaded_render, cache, buffer, it_count, 0, 2 * _d, canvas_size * supersampling, (3 * _d), 3 };
		t4 = std::thread{ threaded_render, cache, buffer, it_count, 0, 3 * _d, canvas_size * supersampling, (4 * _d), 4 };
		t1.join();																		   
		t2.join();																		   
		t3.join();																		   
		t4.join();																		   
		break;																			   
	case 8:																				   
		t1 = std::thread{ threaded_render, cache, buffer, it_count, 0,      0, canvas_size * supersampling,      _d , 1};
		t2 = std::thread{ threaded_render, cache, buffer, it_count, 0,     _d, canvas_size * supersampling, (2 * _d), 2};
		t3 = std::thread{ threaded_render, cache, buffer, it_count, 0, 2 * _d, canvas_size * supersampling, (3 * _d), 3};
		t4 = std::thread{ threaded_render, cache, buffer, it_count, 0, 3 * _d, canvas_size * supersampling, (4 * _d), 4};
		t5 = std::thread{ threaded_render, cache, buffer, it_count, 0, 4 * _d, canvas_size * supersampling, (5 * _d), 5};
		t6 = std::thread{ threaded_render, cache, buffer, it_count, 0, 5 * _d, canvas_size * supersampling, (6 * _d), 6};
		t7 = std::thread{ threaded_render, cache, buffer, it_count, 0, 6 * _d, canvas_size * supersampling, (7 * _d), 7};
		t8 = std::thread{ threaded_render, cache, buffer, it_count, 0, 7 * _d, canvas_size * supersampling, (8 * _d), 8};
		t1.join();
		t2.join();
		t3.join();
		t4.join();
		t5.join();
		t6.join();
		t7.join();
		t8.join();
		break;
	case 16:
		t1  = std::thread{ threaded_render, cache, buffer, it_count, 0,      0, canvas_size * supersampling,       _d  , 1};
		t2  = std::thread{ threaded_render, cache, buffer, it_count, 0,     _d, canvas_size * supersampling, (2 * _d)  , 2};
		t3  = std::thread{ threaded_render, cache, buffer, it_count, 0, 2 * _d, canvas_size * supersampling, (3 * _d)  , 3};
		t4  = std::thread{ threaded_render, cache, buffer, it_count, 0, 3 * _d, canvas_size * supersampling, (4 * _d)  , 4};
		t5  = std::thread{ threaded_render, cache, buffer, it_count, 0, 4 * _d, canvas_size * supersampling, (5 * _d)  , 5};
		t6  = std::thread{ threaded_render, cache, buffer, it_count, 0, 5 * _d, canvas_size * supersampling, (6 * _d)  , 6};
		t7  = std::thread{ threaded_render, cache, buffer, it_count, 0, 6 * _d, canvas_size * supersampling, (7 * _d)  , 7};
		t8  = std::thread{ threaded_render, cache, buffer, it_count, 0, 7 * _d, canvas_size * supersampling, (8 * _d)  , 8};
		t9  = std::thread{ threaded_render, cache, buffer, it_count, 0, 8 * _d, canvas_size * supersampling, (9 * _d)  , 9};
		t10 = std::thread{ threaded_render, cache, buffer, it_count, 0, 9 * _d, canvas_size * supersampling, (10 * _d) , 10 };
		t11 = std::thread{ threaded_render, cache, buffer, it_count, 0,10 * _d, canvas_size * supersampling, (11 * _d) , 11 };
		t12 = std::thread{ threaded_render, cache, buffer, it_count, 0,11 * _d, canvas_size * supersampling, (12 * _d) , 12 };
		t13 = std::thread{ threaded_render, cache, buffer, it_count, 0,12 * _d, canvas_size * supersampling, (13 * _d) , 13 };
		t14 = std::thread{ threaded_render, cache, buffer, it_count, 0,13 * _d, canvas_size * supersampling, (14 * _d) , 14 };
		t15 = std::thread{ threaded_render, cache, buffer, it_count, 0,14 * _d, canvas_size * supersampling, (15 * _d) , 15 };
		t16 = std::thread{ threaded_render, cache, buffer, it_count, 0,15 * _d, canvas_size * supersampling, (16 * _d) , 16 };
		t1.join();
		t2.join();
		t3.join();
		t4.join();
		t5.join();
		t6.join();
		t7.join();
		t8.join();
		t9.join();
		t10.join();
		t11.join();
		t12.join();
		t13.join();
		t14.join();
		t15.join();
		t16.join();
		break;
	}

	THREADS_RDY = 1;
//	std::cout << std::endl;
}

Color get(int** buffer, int x, int y)
{
	if (x < 0 || x >= canvas_size * supersampling || y < 0 || y >= canvas_size * supersampling) return in;
	else if (buffer[x][y] > -1) return LUT[buffer[x][y] % gradient_breaks];
	else return in;
}

void antialias(GLFWwindow * window, bool refresh, int** buffer, int size)
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
						c = c + get(buffer, i + k, j + l);
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
	LUT = new Color[gradient_breaks];
	for (int i = 0; i < gradient_breaks; i++)
		LUT[i] = Lerp(border, out, i);
}

void Redraw()
{
	//if (THREADS_RDY)
	{
		it = 1;
		aa_complete = false;

		for (int i = 0; i < threads; i++)
			REDRAW_FLAG |= 1 << i;

		/*for (int i = 0; i < canvas_size * supersampling; i++)
		{
			for (int j = 0; j < canvas_size * supersampling; j++)
			{
				buffer[i][j] = -1;
				a[i][j].im = a[i][j].real = 0;
			}
		}*/
		std::system("cls");
		std::cout << "\nREDRAW INITIATED\n\n"; 
		begin = std::chrono::steady_clock::now();
	}

}

double l_x, l_y;
void mouse_move_callback(GLFWwindow * window, double xpos, double ypos)
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

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		double delta = 5 / (canvas_size * zoom * supersampling);

		double d_x = (canvas_size/2) - xpos, d_y = (canvas_size / 2) - ypos;

		pt_x -= d_x * delta;
		pt_y -= d_y * delta;

		Redraw();
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (yoffset > 0)
	{
		zoom *= 1.2;
	}
	else if (yoffset < 0)
	{
		zoom /= 1.2;
	}
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
	/*glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glHint(GL_POINT_SMOOTH, GL_NICEST);
	glHint(GL_LINE_SMOOTH, GL_NICEST);
	glHint(GL_POLYGON_SMOOTH, GL_NICEST);

	glEnable(GL_POINT_SMOOTH);*/
	glfwSetCursorPosCallback(window, mouse_move_callback); 
	glfwSetScrollCallback(window, scroll_callback); 
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, canvas_size, canvas_size, 0, 0, 1);

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

	int downsample = 1;
	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		it_complete = !(it <= iteration_limit);

		if (!it_complete)
		{
			pixel_iteration(a, buffer, zoom, pt_x, pt_y, it++);
			if (it % 50 == 0)
				std::cout << "\rIteration " << it;

			if (it == 30) renderloop(window, buffer, true, 5);
			else if (it == 50) renderloop(window, buffer, true, 4);
			else if (it == 100) renderloop(window, buffer, true, 3);
			else if (it == 250) renderloop(window, buffer, true, 2);
			else if (it == 500) renderloop(window, buffer, true, 1);
			else if (it >= 1000 && it % 1000 == 0) renderloop(window, buffer, true, 1);
			else
				glfwPollEvents();
		}
		else                // Iterations done
		{
			if (!aa_complete)
			{  
				std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
				renderloop(window, buffer, true, 1);
				std::cout << "\r" << iteration_limit << " iterations complete.\n";
				std::cout << "Iteration time = " << (float)(std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count())/1000000 << " sec." << std::endl;
				std::cout << "\nAntialiasing started." << std::endl;
				antialias(window, 1, buffer, 1);
				aa_complete = true;
				std::chrono::steady_clock::time_point aa_end = std::chrono::steady_clock::now();
				std::cout << "Antialiasing complete." << std::endl;
				std::cout << "Antialiasing time = " << (float)(std::chrono::duration_cast<std::chrono::microseconds>(aa_end - end).count()) / 1000000 << " sec." << std::endl;
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