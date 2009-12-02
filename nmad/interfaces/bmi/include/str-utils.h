/*
 * This file has been imported from PVFS2 implementation of BMI
 *
 * NewMadeleine
 * Copyright (C) 2009 (see AUTHORS file)
 */


#ifndef STR_UTILS_H
#define STR_UTILS_H

/* BMI is only available for 'huge tags' (ie. at least 64 bits) */
#ifdef NM_TAGS_AS_INDIRECT_HASH

/*
 * BMI_split_string_list()
 *
 * separates a comma delimited list of items into an array of strings
 *
 * returns the number of strings successfully parsed
 */
int BMI_split_string_list(char ***tokens, const char *comma_list)
{

    const char *holder = NULL;
    const char *holder2 = NULL;
    const char *end = NULL;
    int tokencount = 1, retval;
    int i = -1;

    if (!comma_list || !tokens)
    {
	return (0);
    }

    /* count how many commas we have first */
    holder = comma_list;
    while ((holder = index(holder, ',')))
    {
	tokencount++;
	holder++;
    }

    /* if we don't find any commas, just set the entire string to the first
     *  token and return
     */
    if(0 == tokencount)
    {
        tokencount = 1;
    }

    retval = tokencount;
    /* allocate pointers for each */
    *tokens = (char **) malloc(sizeof(char *) * tokencount);
    if (!(*tokens))
    {
	return 0;
    }

    if(1 == tokencount)
    {
	(*tokens)[0] = strdup(comma_list);
	if(!(*tokens)[0])
	{
	    tokencount = 0;
	    goto failure;
	}
	return tokencount;
    }

    /* copy out all of the tokenized strings */
    holder = comma_list;
    end = comma_list + strlen(comma_list);
    for (i = 0; i < tokencount && holder; i++)
    {
	holder2 = index(holder, ',');
	if (!holder2)
	{
	    holder2 = end;
	}
        if (holder2 - holder == 0) {
            retval--;
            goto out;
        }
	(*tokens)[i] = (char *) malloc((holder2 - holder) + 1);
	if (!(*tokens)[i])
	{
	    goto failure;
	}
	strncpy((*tokens)[i], holder, (holder2 - holder));
	(*tokens)[i][(holder2 - holder)] = '\0';
        assert(strlen((*tokens)[i]) != 0);
	holder = holder2 + 1;
    }

out:
    return (retval);

  failure:

    /* free up any memory we allocated if we failed */
    if (*tokens)
    {
	for (i = 0; i < tokencount; i++)
	{
	    if ((*tokens)[i])
	    {
		free((*tokens)[i]);
	    }
	}
	free(*tokens);
    }
    return (0);
}

/* BMI_free_string_list()
 * 
 * Free the string list allocated by PINT_split_string_list()
 */
void BMI_free_string_list(char ** list, int len)
{
    int i = 0;

    if(list)
    {
        for(; i < len; ++i)
        {
            if(list[i])
            {
                free(list[i]);
            }
        }
        free(list);
    }
}

#endif /* NM_TAGS_AS_INDIRECT_HASH */

#endif	/* STR_UTILS_H */
