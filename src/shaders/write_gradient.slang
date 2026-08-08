
import utils;

struct PushConstant
{
	uint idk;	
};

[[vk::push_constant]]
PushConstant pc;

[shader("compute")]
[numthreads(64, 1, 1)]
void computeMain(uint index : SV_DispatchThreadID)
{
	
}
