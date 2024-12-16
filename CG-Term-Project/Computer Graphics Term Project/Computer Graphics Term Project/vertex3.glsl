#version 330

//--- in_Position: attribute index 0 
//--- in_Color: attribute index 1

layout(location = 0) in vec3 in_Position; //--- ��ġ ����: attribute position 0
layout(location = 1) in vec3 in_Color; //--- �÷� ����: attribute position 1
layout(location = 2) in vec3 vNormal;

out vec3 out_Color; //--- �����׸�Ʈ ���̴����� ����
out vec3 FragPos;
out vec3 Normal;

uniform mat4 model; //--- �𵨸� ��ȯ��: ���� ���α׷����� ���� ? uniform ������ ����: ���� �̸���model���� �޾ƿ�
uniform mat4 view; //--- ���� ��ȯ��: ���� ���α׷����� ���� ? uniform ������ ����: ���� �̸���view���� �޾ƿ�
uniform mat4 projection; //--- ���� ��ȯ��: ���� ���α׷����� ���� ? uniform ������ ����: ���� �̸���projection���� �޾ƿ�

void main(void)
{
	gl_Position = projection * view * model * vec4(in_Position.x, in_Position.y, in_Position.z, 1.0);
	FragPos = vec3(model*vec4(in_Position, 1.0));
	Normal = vec3(model*vec4(vNormal, 0.0));
	out_Color = in_Color;
}