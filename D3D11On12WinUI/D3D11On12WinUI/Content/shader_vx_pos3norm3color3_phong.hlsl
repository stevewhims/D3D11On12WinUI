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

cbuffer WorldViewProjectionConstantBuffer : register(b0)
{
	matrix World;
	matrix View;
	matrix Projection;
};

// Per-vertex data used as input to the vertex shader.
struct VertexShaderInput
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float3 Color : COLOR;
};

// Per-pixel color data passed through the pixel shader.
struct VertexShaderOutput
{
	float4 Position : SV_POSITION;
	float3 WorldNormal : NORMAL0;
	float3 ViewNormal : NORMAL1;
	float3 Color : COLOR;
};

VertexShaderOutput main(VertexShaderInput input)
{
	VertexShaderOutput output;
	float4 position = { input.Position, 1 };

	// Transform the vertex position into projected space.
	position = mul(position, World);
	position = mul(position, View);
	position = mul(position, Projection);
	output.Position = position;

	float3 normal = input.Normal;

	// Transform the vertex normal into world space...
	normal = mul(normal, (float3x3)World);
	output.WorldNormal = normal;
	// ...and view space.
	normal = mul(normal, (float3x3)View);
	output.ViewNormal = normal;

	// Pass the color through without modification.
	output.Color = input.Color;

	return output;
}
