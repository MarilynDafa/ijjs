#include <libwebsockets.h>
#include <uv.h>
int lws_settings_plat_get(lws_settings_instance_t* si, const char* name, uint8_t* dest, size_t* max_actual)
{
	char buf[1024] = {0};
	size_t size = sizeof(buf);
	uv_os_tmpdir(buf, &size);
	strcat(buf, "/");
	strcat(buf, name);
	FILE* fp = fopen(buf, "rb");
	if (!fp)
		return 1;
	unsigned int l = 0;
	fread(&l, 1, sizeof(int), fp);
	fread(dest, 1, l, fp);
	*max_actual = l;
	fclose(fp);
	return 0;
}

int lws_settings_plat_set(lws_settings_instance_t* si, const char* name, const uint8_t* src, size_t len)
{
	char buf[1024] = { 0 };
	size_t size = sizeof(buf);
	uv_os_tmpdir(buf, &size);
	strcat(buf, "/");
	strcat(buf, name);
	FILE* fp = fopen(buf, "wb");
	unsigned int l = len;
	fwrite(&l, 1, sizeof(int), fp);
	fwrite(src, 1, len, fp);
	fclose(fp);
	return 0;
}
