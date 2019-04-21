
namespace
{
	class Color
	{
	public:
		double r, g, b;
		Color(double r, double g, double b)
		{
			this->r = r;
			this->g = g;
			this->b = b;
		}

		Color(int r, int g, int b)
		{
			this->r = (double)r / (double)255;
			this->g = (double)g / (double)255;
			this->b = (double)b / (double)255;
		}
		Color(const char* hex)
		{
			int i = 0;
			if (hex[0] == '#')
				i = 1;

			this->r = (double)((h2i(hex[i]) * 16) + h2i(hex[++i]))/(double)255;
			this->g = (double)((h2i(hex[++i])* 16) + h2i(hex[++i]))/(double)255;
			this->b = (double)((h2i(hex[++i])* 16) + h2i(hex[++i]))/(double)255;
		}

		Color()
		{
			r = g = b = 0;
		}

		Color operator + (Color const& obj)
		{
			Color res;
			res.r = r + obj.r;
			res.g = g + obj.g;
			res.b = b + obj.b;
			return res;
		}
		Color operator / (double const& obj)
		{
			Color res;
			res.r = r / obj;
			res.g = g / obj;
			res.b = b / obj;
			return res;
		}

	private:
		int h2i(char arg)
		{
			switch (arg)
			{
			case '0':
				return 0;
			case '1':
				return 1;
			case '2':
				return 2;
			case '3':
				return 3;
			case '4':
				return 4;
			case '5':
				return 5;
			case '6':
				return 6;
			case '7':
				return 7;
			case '8':
				return 8;
			case '9':
				return 9;
			case 'a':
			case 'A':
				return 10;
			case 'b':
			case 'B':
				return 11;
			case 'c':
			case 'C':
				return 12;
			case 'd':
			case 'D':
				return 13;
			case 'e':
			case 'E':
				return 14;
			case 'f':
			case 'F':
				return 15;
			}
		}
	};
}