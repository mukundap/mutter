#include <glib-object.h>
#include "backends/native/meta-kms-color-utility.h"
#include "backends/native/meta-kms.h"

#define DD_MIN(a, b) ((a) < (b) ? (a) : (b))
#define DD_MAX(a, b) ((a) < (b) ? (b) : (a))
#define MAX_24BIT_NUM ((1<<24) -1)
void create_unity_log_lut(uint32_t NumOfSegments, uint32_t *NumEntriesPerSegment, uint32_t *pOutputLut)
{
	uint32_t temp, index = 0;
	pOutputLut[index++] = 0;
	for (int segment=0; segment < NumOfSegments; segment++)
	{
	        uint32_t entry_count = NumEntriesPerSegment[segment];
	        uint32_t start = (1 << (segment - 1));
	        uint32_t end = (1 << segment);
	        for (uint32_t entry = 1; entry <= entry_count; entry++)
	        {
	                temp = start + entry * ((end - start) * 1.0 / entry_count);
	                pOutputLut[index] = DD_MIN(temp, MAX_24BIT_NUM);
	                index++;
	        }
	}
	g_warning("Sameer: Linear LUTs:\n");
	meta_topic(META_DEBUG_KMS, "Sameer: Linear LUTs:\n");
        for (int i=0; i<513; i++)
        {
                meta_topic(META_DEBUG_KMS, "%d, ", pOutputLut[i]);
        }
        meta_topic(META_DEBUG_KMS, "ALL LUTS OK?\n");
}
