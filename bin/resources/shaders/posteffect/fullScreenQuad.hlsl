struct VOut
{
	float4 pos : SV_POSITION;
	float2 texCoord : TEXCOORD0;
};

VOut VShader(float4 Input : SV_POSITION) {
	VOut output;
	output.pos = Input;
	output.texCoord = (Input.xy * float2(1, -1) + 1.0f) / 2.0f;
	return output;
}

Texture2D texture0 : register(t0);
SamplerState textureSampler : register(s0);

float4 PShader(VOut input) : SV_TARGET {

	float4 textureColor = texture0.Sample(textureSampler, input.texCoord);

	if (textureColor.r > 1) {
		textureColor = float4(1, 0, 0, 1);
	}

	return textureColor;
}
