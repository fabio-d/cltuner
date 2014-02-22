typedef float2 cpx;
#define I ((cpx)(0.0, 1.0))

inline float cabs(cpx a)
{
	return sqrt(a.x*a.x + a.y*a.y);
}

inline cpx cmult(cpx a, cpx b)
{
	return (cpx)(a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x);
}

inline cpx cexp(cpx a)
{
	return exp(a.x) * (cpx)(cos(a.y), sin(a.y));
}
