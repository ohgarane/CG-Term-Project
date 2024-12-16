#version 330 core

//--- out_Color: ���ؽ� ���̴����� �Է¹޴� ���� ��
//--- FragColor: ����� ������ ������ ������ ���۷� ���� ��.

in vec3 out_Color; //--- ���ؽ� ���̴����Լ� ���� ����
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor; //--- ���� ���

uniform vec3 cameraPos;
uniform vec3 lightPos;
uniform vec3 lightColor;

void main(void)
{
	vec3 ambientLight = vec3(0.9f, 0.9f, 0.9f);
	vec3 ambient = ambientLight * lightColor;

	vec3 NV = normalize(Normal);
	vec3 lightDir = normalize(lightPos - FragPos);
	float diffuseLight = max(dot(NV, lightDir), 0.0);
	vec3 diffuse = diffuseLight * lightColor;

	int shininess = 128;
	vec3 viewDir = normalize(cameraPos - FragPos);
	vec3 reflectDir = reflect(lightDir, NV);
	float specularLight = max(dot(viewDir, reflectDir), 0.0);
	specularLight = pow(specularLight, shininess);
	vec3 specular = specularLight * lightColor;

	vec3 result = (diffuse + specular + ambient) * out_Color;
	FragColor = vec4(result, 1.0);
}