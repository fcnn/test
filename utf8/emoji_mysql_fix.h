#ifndef _UGC_EMOJI_FIX_H
#define _UGC_EMOJI_FIX_H

#include <string>

namespace ugc_emoji {
	std::string *encode_emoji(std::string *text);
	std::string *decode_emoji(std::string *text);
} // namespace ugc_emoji
#endif // _UGC_EMOJI_FIX_H
