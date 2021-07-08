//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

// For asm, compile with fxc.exe <this_filename>.hlsl /E main /T ps_4_1 /Fc

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 Position : SV_POSITION;
	float3 WorldNormal : NORMAL0;
	float3 ViewNormal : NORMAL1;
	float3 Color : COLOR;
};

float3 CalcPhong(float3 worldNormal, float3 viewNormal, float3 color)
{
	float3 lightDirection = { 1, -1, -1 };
	float3 ambientLight = { .5f, .3f, .3f };
	float3 directionalLight = { .4, .4, .4f };

	// Calculate the directional illumination...
	float3 lightDirectionNormalized = normalize(lightDirection);
	float3 finalColor = (ambientLight + dot(worldNormal, lightDirectionNormalized) * directionalLight) * color;

	// ... then add the specular.
	float3 viewVector = float3(0, 0, -1);
	float3 halfway = -normalize(viewVector + lightDirection);
	float dotProduct = max(0.f, dot(viewNormal, halfway));
	float specularLuminance = pow(dotProduct, 30);
	finalColor += specularLuminance * directionalLight;

	return finalColor;
}

float4 main(PixelShaderInput input) : SV_TARGET
{
	float3 worldNormal = normalize(input.WorldNormal);
	float3 viewNormal = normalize(input.ViewNormal);
	float3 color = input.Color.rgb * input.Color.rgb;
	float4 finalPixelColor = float4(CalcPhong(worldNormal, viewNormal, color), 1.f);
	return finalPixelColor;
}
