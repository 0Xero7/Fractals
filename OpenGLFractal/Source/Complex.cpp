
namespace
{
	class Complex
	{
	public:
		double real, im;
		Complex()
		{
			real = im = 0;
		}
		Complex(double a, double b)
		{
			real = a;
			im = b;
		}
	};
}