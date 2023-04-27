
#include "postgres.h"
#include "fmgr.h"
#include "libpq/pqformat.h"		/* needed for send/recv functions */
#include "access/hash.h"
#include "utils/builtins.h"
#include <ctype.h>
#include <string.h>

PG_MODULE_MAGIC;

typedef struct personName
{
	int length;
	char pname[1];

}			PersonName;


// check vaild
// TO DO
static int check_input(char *test)
{
	int result = 1;
    int string_len = strlen(test);
    int count_comma = 0;
    int count_invaild = 0;
    int comma_index = 0;
    int letter_len = 0;
    int in_letter = 0;

    for (int i = 0 ; i < string_len ; i++)
    {
        if (test[i]==',')
        {
            count_comma++;
            comma_index = i;
        }
        if (!isalpha(test[i]) && test[i]!='-' && test[i]!= '\'' && test[i]!=' ' && test[i]!= ',')
        {
            count_invaild++;
        }
    }
    if (count_comma == 1 && count_invaild == 0)
    {
        if (isalpha(test[0]) && test[comma_index-1]!=' ' && test[comma_index+2]!=' ' && test[string_len-1]!=' ')
        {
            for (int i = 0; i < string_len ; i++ )
            {
                if (test[i]==' '||test[i]==','){
                    if (letter_len == 1){
                        //result = 0;
                        break;
                    } else {
                        in_letter = 0;
                        letter_len = 0;
                    }
                } else {
                    if (in_letter==0){
                        if (isupper(test[i])){
                            letter_len=1;
                            in_letter=1;
                        } else {
                            result = 0;
                            break;
                        }
                    } else {
                        letter_len ++;
                    }
                }

            }
			if (letter_len == 1)
            {
                result = 0;
            }
        } else {
            result = 0;
        }
    } else {
        result = 0;
    }
    return result;
}
/*****************************************************************************
 * Input/Output functions
 *****************************************************************************/

PG_FUNCTION_INFO_V1(pname_in);

Datum
pname_in(PG_FUNCTION_ARGS)
{
	char	   *str = PG_GETARG_CSTRING(0);
	PersonName    *result;
	int length = 0;
	int comma_index = 0;
    int j = 0;

	if (check_input(str) == 0)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input syntax for type %s: \"%s\"",
						"PersonName", str)));
	
	// remove ' ' after ','
	length = strlen(str);
	for (int i = 0;i<length;i++)
    {
        if (str[i]==','){
            comma_index = i;
            break;
        }
    }
    if (str[comma_index+1]==' '){
        length--;
    }

	result = (PersonName *) palloc(VARHDRSZ + length + 1);
	SET_VARSIZE(result, VARHDRSZ + length + 1);

	for (int i = 0;i<length;i++){
        if(str[j]== ' '&&str[j-1]==',')
        {
            j++;
        }
        result->pname[i] = str[j];
        j++;
    }
    result->pname[length]='\0';
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(pname_out);

Datum
pname_out(PG_FUNCTION_ARGS)
{
	PersonName    *personName = (PersonName *) PG_GETARG_POINTER(0);
	char	   *result;

	result = psprintf("%s", personName->pname);
	PG_RETURN_CSTRING(result);
}

/*****************************************************************************
 * Operator class for defining B-tree index
 *****************************************************************************/

static int
pname_abs_cmp_internal(PersonName * a, PersonName * b)
{
	// Compare two names 
	char *a_family=NULL;
	char *b_family=NULL;
	int a_comma_index=0;
	int b_comma_index=0;

	char *a_given = strchr(a->pname,',')+1;
	char *b_given = strchr(b->pname,',')+1;

	a_comma_index = strlen(a->pname)- strlen(a_given);
	b_comma_index = strlen(b->pname)- strlen(b_given);
	a_family = palloc(sizeof(char) *a_comma_index);
	b_family = palloc(sizeof(char) *b_comma_index);
	strncpy(a_family,a->pname,a_comma_index-1);
    a_family[a_comma_index-1]='\0';
	strncpy(b_family,b->pname,b_comma_index-1);
    b_family[b_comma_index-1]='\0';	
	
	if (strcmp(a_family,b_family)!=0)
	{
		return strcmp(a_family,b_family);
	}else{
		return strcmp(a_given,b_given);
	}
}


PG_FUNCTION_INFO_V1(pname_abs_lt);

Datum
pname_abs_lt(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(pname_abs_cmp_internal(a, b) < 0);
}

PG_FUNCTION_INFO_V1(pname_abs_le);

Datum
pname_abs_le(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(pname_abs_cmp_internal(a, b) <= 0);
}

PG_FUNCTION_INFO_V1(pname_abs_eq);

Datum
pname_abs_eq(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(pname_abs_cmp_internal(a, b) == 0);
}

PG_FUNCTION_INFO_V1(pname_abs_ge);

Datum
pname_abs_ge(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(pname_abs_cmp_internal(a, b) >= 0);
}

PG_FUNCTION_INFO_V1(pname_abs_gt);

Datum
pname_abs_gt(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(pname_abs_cmp_internal(a, b) > 0);
}

PG_FUNCTION_INFO_V1(pname_abs_neq);

Datum
pname_abs_neq(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(pname_abs_cmp_internal(a, b) != 0);
}


PG_FUNCTION_INFO_V1(pname_abs_cmp);

Datum
pname_abs_cmp(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	PersonName    *b = (PersonName *) PG_GETARG_POINTER(1);

	PG_RETURN_INT32(pname_abs_cmp_internal(a, b));
}


PG_FUNCTION_INFO_V1(family);

Datum
family(PG_FUNCTION_ARGS)
{
	char *result = NULL;
	int comma_index = 0;
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);

	// take the family name  
	for (int i = 0; i<strlen(a->pname);i++)
	{
		if (a->pname[i]==',')
		{
			comma_index = i;
			break;
		}
	}
	result = palloc (sizeof(char)* (comma_index+1));
	strncpy(result,a->pname,comma_index);
	result[comma_index] = '\0';

	PG_RETURN_TEXT_P(cstring_to_text(result));

}

PG_FUNCTION_INFO_V1(given);

Datum
given(PG_FUNCTION_ARGS)
{
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	// take the given name 
	char *result = strchr(a->pname,',')+1;
	PG_RETURN_TEXT_P(cstring_to_text(result));
}

PG_FUNCTION_INFO_V1(show);

Datum
show(PG_FUNCTION_ARGS)
{
	char *result = NULL;
	char *family = NULL;
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);
	int comma_index = 0;
	int given_length = 0;
	int first_length = 0;
	int result_length = 0;
	int pname_length = strlen(a->pname);

	// take the given name 
	char *given = strchr(a->pname,',')+1;
	given_length = strlen(given);
	comma_index = pname_length - given_length;
	family = palloc(sizeof(char)*comma_index);
	strncpy(family,a->pname,comma_index-1);
	family[comma_index-1]='\0';

    // get first letter of given name
	for (int i = 0;i< strlen(given);i++){
        if (given[i]==' '){
            break;
        } else {
            first_length ++;
        }
    }

	result_length = first_length + 1 + comma_index;

	result = palloc(sizeof(char)*result_length);
	strncpy(result,given,first_length);
	result[first_length] = ' ';

	strcpy(result+first_length+1,family);
	result[result_length-1]='\0';
	PG_RETURN_TEXT_P(cstring_to_text(result));
	
}


PG_FUNCTION_INFO_V1(pname_hash);

Datum
pname_hash(PG_FUNCTION_ARGS)
{
	int hash_code = 0;
	PersonName    *a = (PersonName *) PG_GETARG_POINTER(0);

	hash_code = DatumGetUInt32(hash_any((const unsigned char *) a->pname,strlen(a->pname))); 

	PG_RETURN_INT32(hash_code);
}

