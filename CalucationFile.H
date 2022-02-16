#include "string.h"
#include <stdint.h>
#include <unistd.h>

		 
uint8_t   	BytesCheckSum(uint8_t *,uint8_t);//闁哄稄绻濋悰娆撳椽鐏炲墽妲烽弶鍫嫹
uint8_t     BytesComp( uint8_t * ,uint8_t *,uint8_t);//閻庢稒顨堥浣圭▔閸欏妲烽弶鍫濆枦缁辨繂顫㈤敐鍥ｂ偓妯绘交閺傛寧绀€0
uint8_t	    BCD_String_Diag(uint8_t * ,uint8_t);//BCD闁活喕绀侀悺褏绮敂鑳洬閻犲洤锕ラ弻锟�
uint8_t     BCDToHex(uint8_t  );
uint8_t     HexToBCD(uint8_t  );
uint32_t	ChgBCDStringToUlong(uint8_t * ,uint8_t);//BCD闁活喕绀侀悺褏绮敂鑳洬閺夌儐鍓氬畷鍙夌▔閻戞ɑ娈婚柛銊ヮ儐閺嗭拷
uint8_t	    CalCheckSum(uint8_t * ,uint8_t);//閻犱緤绱曢悾濠氬冀閿熺姷宕ｉ柛婊冪焿閹凤拷
void	    ChgUlongToBCDString( uint32_t ,uint8_t * ,uint8_t );
void	    ChgIntToStrings(uint16_t	ii,uint8_t * ptr);
uint32_t	ChgInputToUlong(uint8_t * ,uint8_t);//閺夊牊鎸搁崣鍡涙儍閸曨剚娈堕悗娑欘殙濞村棝骞戦～顓＄闂傗偓閹稿孩娈荤憸甯嫹
uint32_t 	GetU32_HiLo(uint8_t *);
uint16_t 	GetU16_HiLo(uint8_t *);
void 	    PutU32_HiLo(uint8_t *,uint32_t);
void 	    PutU16_HiLo(uint8_t *,uint16_t);
void	    FormatBuffer(uint8_t,uint8_t * ,uint8_t *);
uint32_t	ChgKeyStringToUlong(uint8_t * ptr ,uint8_t Len);
uint32_t	ChgStringsToInt(uint8_t * ptr);
uint16_t    DoubleBigToSmall(uint16_t a);
void        HexGroupToHexString(uint8_t *data,uint8_t *dst,uint8_t len);
uint16_t    HexStringToHexGroup(uint8_t *,uint8_t *,uint16_t);
uint8_t     AddQuFan(uint8_t *str,uint8_t len);//缂侀硸鍨版慨鐐哄椽鐏炶棄绲块柛娆欐嫹
uint32_t    ChgBCDStringTouint32_t(uint8_t * ptr ,uint8_t Len);//BCD閻礁鐡х粭锔胯鏉烆剚宕叉稉鐑樻殻閸ㄥ鏆�
void        Chguint32_tToBCDString( uint32_t iii,uint8_t * ptr,uint8_t Len);
void        ChgTimeToRecordDatas(uint8_t * Timeptr,uint8_t * ptr);
 