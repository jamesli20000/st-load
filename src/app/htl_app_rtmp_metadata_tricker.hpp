#ifndef HTL_APP_RTMP_METADATA_TRICKER_HPP
#define HTL_APP_RTMP_METADATA_TRICKER_HPP

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include "htl_app_rtmp_protocol.hpp"
#include "htl_core_log.hpp"

 union av_intfloat64 {
     uint64_t i;
     double f;
 };
 
 enum script_data_type {
     Number = 0,
     Boolean,
     String,
     Object,
     MovieClip,
     Null,
     Undefined,
     Reference,
     EcmaArray,
     ObjectEndMarker,
     StringArray,
     Date,
     LongString,
 };
 typedef enum script_data_type script_type_t;
 
 struct script_string {
     char len[2];
 };
 typedef struct script_string String_t;
 
 struct flv_header {
     unsigned char tag_f;
     unsigned char tag_l;
     unsigned char tag_v;
     unsigned char version;
     unsigned char audio_and_video;
     unsigned char DataOffset[4];
 };
 typedef struct flv_header header_t;
 
 struct flv_tag {
     unsigned char tagtype;
     unsigned char size[3];
     unsigned char timestamp[3];
     unsigned char extent_timestamp;
     unsigned char stream_id[3];
 };
 typedef struct flv_tag tag_t;
 
 struct flv_body {
     tag_t tag;
     // unsigned char PreviousTagSize[2];
 };
 typedef struct flv_body body_t;
 
 struct object_name {
     unsigned short length;
     unsigned char name;
 };
 typedef struct object_name object_name_t;
 
 struct script_object {
     object_name_t name;
 };
 typedef struct script_object_name script_object_t;
 
 
 /**
  * Reinterpret a 64-bit integer as a double.
  */
 static double int2double(uint64_t i)
 {
     union av_intfloat64 v;
     v.i = i;
     return v.f;
 }
 
 /*
  * get_header_audio check if the flv have audio stream
  * info, a byte with video and audio bit flag
  *
  * return 0 or 1
 */
 int get_header_audio(char info)
 {
     return (info & 0x4) >> 2;
 }
 
 /*
  * get_header_video check if the flv have video stream
  * info, a byte with video and audio bit flag
  *
  * return 0 or 1
 */
 int get_header_video(char info)
 {
     return info & 0x1;
 }
 
 /*
  * flv_header_parse just parse the FLV VERSION tag
  * fd, file describe handle
  * return flv version
 */
 int flv_header_parse(int fd)
 {
     int ret = 0;
     header_t header;
 
     memset(&header, 0, sizeof(header));
     ret = read(fd, &header, sizeof(header));
     if (ret <= 0) {
         Error("read header error, %d\n", -EINVAL);
         return -EINVAL;
     }
 
 
     Info("flv = [%c %c %c] version = [%x]\n"
             "TypeFlagsAudio = [%x], TypeFlagsVideo = [%x], DataOffset = [%x %x %x %x]\n",
             header.tag_f, header.tag_l, header.tag_v, header.version,
             get_header_audio(header.audio_and_video), get_header_video(header.audio_and_video),
             header.DataOffset[0], header.DataOffset[1], header.DataOffset[2], header.DataOffset[3]);
 
     return (int)header.version;
 }


 
 /*
  * flv_do_tag just parse all the flv tag
  * fd, file describe handle
  * size, the size of the flv tag body
  *
  * return tag type
 */
 int flv_do_tag(int fd, unsigned long *size)
 {
     int ret = 0;
     char *p = NULL;
     char data_size[11] = {0};
     body_t body;
 
 
     ret = read(fd, &body, sizeof(body));
     if (ret <= 0) {
         Error("Can not ret the fd errno [%d]\n", -EINVAL);
         return -EINVAL;
     }
 
     Info("size = %lu tag->type = [%x]"
             "tag->size = [%.2x %.2x %.2x], tag->timestamp = [%.2x %.2x %.2x], "
             "tag->extent_timestamp = [%.2x], tag->streamid = [%.2x %.2x %.2x]\n",
             sizeof(body), body.tag.tagtype,
             body.tag.size[0], body.tag.size[1],body.tag.size[2], body.tag.timestamp[0], body.tag.timestamp[1],
             body.tag.timestamp[2], body.tag.extent_timestamp, body.tag.stream_id[0],
             body.tag.stream_id[1], body.tag.stream_id[2]);
     memcpy(data_size, body.tag.size, 3);
     p = data_size;
     snprintf(data_size, sizeof(data_size), "0x%x%x%x", body.tag.size[0], body.tag.size[1], body.tag.size[2]);
     *size = strtoul(p, NULL, 16);
     Info("size = [%lu] \n", *size);
 
     return body.tag.tagtype;
 }
 
 /*
  * get_string_len get the key-value value string length
  * data, the data will be parsed
  *
  * return the String length by int
 */
 int get_string_len(unsigned char *data)
 {
 
     unsigned char *p = data;
     unsigned char len_char[8];
     int len_int = 0;
 
     memset(len_char, 0, sizeof(len_char));
     snprintf((char *)len_char, sizeof(len_char), "0x%x%x", data[0], data[1]);
     p = len_char;
 
     len_int = strtoul((char *)p, NULL, 16);
 
     return len_int;
 }
 
 /*
  * get_bool_value get bool type value (key-value value)
  * data, the data will be parsed
  *
  * return bool value 0 or 1
 */
 int get_bool_value(unsigned char *data)
 {
    return *data;
 }
 
 /*
  * get_key_len the key-value key string length unit: four bytes
  * data, the data will be parsed
  *
  * return the key length by int
 */
 int get_key_len(unsigned char *data)
 {
     char len_char[16];
     char *p = len_char;
 
     memset(len_char, 0, sizeof(len_char));
     snprintf(len_char, sizeof(len_char), "0x%x%x%x%x", data[0], data[1], data[2], data[3]);
 
     return strtoul(p, NULL, 16);
 }

 /*
  * process_ecma_array The onMetaData sub type named ECMA Array
  * data, the data will be parsed
  *
  * return the offset of the data
 */
 int script_type_parse(unsigned char *data, bool bdur);
 int process_ecma_array(unsigned char *data)
 {
     int ecma_array_len = 0;
     int keyname_len = 0;
     unsigned char keyname[32];
     unsigned char *p = data;
     int i = 0;
 
     ecma_array_len = get_key_len(p);
     p += 4;
 
     for (i = 0; i < ecma_array_len; i++) {
         keyname_len = get_string_len(p);
         p += 2;
         memset(keyname, 0, sizeof(keyname));
         strncpy((char *)keyname, (const char *)p, keyname_len);
         Info("name = [%s]\n", keyname);
         p += keyname_len;
         bool bdur = false;
         if( !strcmp((char*)keyname, "duration")) bdur = true;
         p += script_type_parse(p, bdur);
     }
 
     if (*p == 0 && *(p + 1) == 0 && *(p + 2) == 9) {
         p += 3;
     }
     return p - data;
 }
 
 /*
  * get_double get the double number by long long type
  * data, the data will be parsed
  *
  * return the parse out number of long long
 */
 unsigned long long get_double(unsigned char *data)
 {
     unsigned char double_number[64];
     unsigned char *p = double_number;
 
     memset(double_number, 0, sizeof(double_number));
     snprintf((char *)double_number, sizeof(double_number), "0x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",
              *data, *(data + 1), *(data + 2), *(data + 3), *(data + 4), *(data + 5), *(data + 6), *(data + 7));
 
     return strtoull((char *)p, NULL, 16);
 
 }
 
 int script_type_parse(unsigned char *data, bool bdur)
 {
     int ret = 0;
     unsigned char *p = data;
     unsigned char string_output[512];
     unsigned long long number = 0;
     double double_number = 0.0;
 
     switch (*p) {
     case Number:
         p++;
         number = get_double(p);
         double_number = int2double(number);
         Info("number = [%.2f]\n", double_number);
         if(bdur)
         {
             g_duration = double_number;
             double ddur = -1;
             int64_t inewdur;
             memcpy(&inewdur, &ddur, 8);
             char*q = (char*)&inewdur;
             *p = q[7];
             *(p+1) = q[6];
             *(p+2) = q[5];
             *(p+3) = q[4];
             *(p+4) = q[3];
             *(p+5) = q[2];
             *(p+6) = q[1];
             *(p+7) = q[0];
             number = get_double(p);
             double_number = int2double(number);
             Info("newnumber = [%.2f]\n", double_number);
         }
         p += 8;
         break;
 
     case Boolean:
         p++;
         ret = get_bool_value(p);
         p++;
         break;
 
     case String:
         p++;
         memset(string_output, 0, sizeof(string_output));
         ret = get_string_len(p);
         p += 2;
         strncpy((char *)string_output, (const char *)p, ret);
         Info("String = [%s]\n", string_output);
         p += ret;
         break;
 
     case Object:
         p++;
         break;
 
     case MovieClip:
         p++;
         break;
 
     case Null:
         p++;
         break;
 
     case Undefined:
         p++;
         break;
 
     case Reference:
         p++;
         break;
 
     case EcmaArray:
         p++;
         ret = process_ecma_array(p);
         p += ret;
         break;
 
     case ObjectEndMarker:
         p++;
         break;
 
     case StringArray:
         p++;
         break;
 
     case Date:
         p++;
         break;
 
     case LongString:
         p++;
         break;
 
     default:
 
         break;
     }
     return p - data;
 }
 
 void ResetDuration4Live(unsigned char*data, int size)
 {
     int i = 0;
     while( i < size )
     {
         i+= script_type_parse(data+i, false);
     }
 }

#endif
