
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
	};
}