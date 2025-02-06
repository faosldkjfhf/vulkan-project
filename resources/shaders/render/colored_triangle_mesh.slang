struct Vertex {
  float3 position;
  float2 uv;
  float3 normal;
  float4 color;
};

[vk::push_constant]
cbuffer Constants {
  float4x4 renderMatrix;
  ConstBufferPointer<Vertex> vertices;
};

struct VertexOutput {
  float4 position : SV_Position;
  float2 uv : TEXCOORD;
  float4 color : COLOR;
};

[shader("vertex")]
VertexOutput vertMain(int vertexIndex: SV_VertexID) {
  Vertex v = vertices[vertexIndex];

  VertexOutput output;

  output.position = mul(transpose(renderMatrix), float4(v.position, 1.0));
  output.color = v.color;
  output.uv = v.uv;

  return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput input) : SV_Target {
  float4 color = input.color;

  return color;
}

