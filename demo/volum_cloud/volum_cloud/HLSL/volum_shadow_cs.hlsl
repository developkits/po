
float3 adject_view_ray(float3 n)
{
	float max_factor = max(max(abs(n.x), abs(n.y)), abs(n.z));
	return n / max_factor;
}

Texture3D tex :register(t[0]);
RWTexture3D<float> shadow_te : register(u[0]);


[numthreads(1, 1, 1)]
void main(uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID, uint groupIndex : SV_GroupIndex, uint3 dispatchThreadID : SV_DispatchThreadID)
{
	float3 light = adject_view_ray(float3(0.0, 1.0, 0.0));
	float len = length(light) / 255.0;
	float3 poi = groupID;
	float last_sam = tex[poi].x;
	float l = 0.0;
	for (uint co = 1; co < 256; ++co)
	{
		float3 curpoi = poi + co * light;
		if (
			curpoi.x <= 256.0 && curpoi.x >= 0.0 &&
			curpoi.y <= 256.0 && curpoi.y >= 0.0 &&
			curpoi.z <= 256.0 && curpoi.z >= 0.0
			)
		{
			float now_sam = tex[curpoi].x;
			l = l - log((last_sam + now_sam) / 2.0) * len;
			last_sam = now_sam;
		}
		else break;
	}
	shadow_te[groupID] = exp(-l);
}