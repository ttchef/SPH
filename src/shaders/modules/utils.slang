
module utils;

public float3 gamma_correct(float3 color)
{
	return pow(color, float3(1.0 / 2.2));	
}
