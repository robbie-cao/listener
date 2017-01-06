typedef struct
{
	char *key;
	int value;
} sf_key_value_t;

extern sf_key_value_t formats[];
extern sf_key_value_t subtypes[];

SF_INFO *sf_parse_settings(char *settings);
sf_key_value_t * sf_get_formats();
sf_key_value_t * sf_get_subtypes();
