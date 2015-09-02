// this is to solve the issue that mysql does not support 4+ bytes utf8

#include "emoji_mysql_fix.h"

#include <string>
#include <iostream>

namespace ugc_emoji {
std::string *encode_emoji(std::string *text) {
	auto p = text->c_str();
	auto len = text->size();
	for (unsigned i = 0; i < len; ) {
		auto utf_octet = 1;
		int unicode = p[i]&0xff;
		if (unicode < 0b10000000) {
			++i;
		} else if (unicode < 0b11100000) {
			i += 2;
		} else if (unicode < 0b11110000) {
			i += 3;
		} else if (unicode < 0b11111000) {
			utf_octet = 4;
			if (i + utf_octet > len) break;
			unicode = (unicode&0b00000111)<<18;
			unicode |= (p[i+1]&0b00111111)<<12;
			unicode |= (p[i+2]&0b00111111)<<6;
			unicode |= (p[i+3]&0b00111111);
		} else if (unicode < 0b11111100) {
			utf_octet = 5;
			if (i + utf_octet > len) break;
			unicode = (unicode&0b00000011)<<24;
			unicode |= (p[i+1]&0b00111111)<<18;
			unicode |= (p[i+2]&0b00111111)<<12;
			unicode |= (p[i+3]&0b00111111)<<6;
			unicode |= (p[i+4]&0b00111111);
		} else if (unicode < 0b11111110) {
			utf_octet = 6;
			if (i + utf_octet > len) break;
			unicode = (unicode&0b00000001)<<30;
			unicode |= (p[i+1]&0b00111111)<<24;
			unicode |= (p[i+2]&0b00111111)<<18;
			unicode |= (p[i+3]&0b00111111)<<12;
			unicode |= (p[i+4]&0b00111111)<<6;
			unicode |= (p[i+5]&0b00111111);
		} else {
			++i;
		}
		if (utf_octet >= 4) {
			char buf[16];
			sprintf(buf, "\\U%08X", unicode);
			text->replace(i, utf_octet, buf, 10);
			i += 10;
			p = text->c_str();
			len = text->size();
		}
	}
	return text;
}

std::string *decode_emoji(std::string *text) {
	auto p = text->c_str();
	auto len = text->size();
	for (unsigned i = 0; i + 10 <= len; ) {
		if (p[i] != '\\' || p[i+1] != 'U') {
			++i;
			continue;
		}
		unsigned int unicode = 0;
		for (auto j = i + 2; j < i + 10; ++j) {
			int hex = p[j];
			if (hex >= '0' && hex <= '9') {
				hex -= '0';
			} else if (hex >= 'A' && hex <= 'F') {
				hex = hex - 'A' + 10;
			} else {
				unicode = 0;
				break;
			}
			unicode <<= 4;
			unicode |= hex;
		}
		if (unicode == 0 || unicode > 0x7FFFFFFF) {
			++i;
			continue;
		}
		char utf[8];
		auto utf_len = 0;
		if (unicode >= 0x10000 && unicode < 0x200000) {
			utf_len = 4;
			utf[0] = 0b11110000|((unicode>>18)&0b00000111);
			utf[1] = 0b10000000|((unicode>>12)&0b00111111);
			utf[2] = 0b10000000|((unicode>>6)&0b00111111);
			utf[3] = 0b10000000|(unicode&0b00111111);
		} else if (unicode < 0x80) {
			utf[0] = unicode;
			utf_len = 1;
		} else if (unicode < 0x800) {
			utf_len = 2;
			utf[0] = 0b10000000|((unicode>>6)&0b00011111);
			utf[1] = 0b10000000|(unicode&0b00111111);
		} else if (unicode < 0x10000) {
			utf_len = 3;
			utf[0] = 0b10000000|((unicode>>12)&0b00001111);
			utf[1] = 0b10000000|((unicode>>6)&0b00111111);
			utf[2] = 0b10000000|(unicode&0b00111111);
		} else if (unicode < 0x4000000) {
			utf_len = 5;
			utf[0] = 0b11110000|((unicode>>24)&0b00000011);
			utf[1] = 0b10000000|((unicode>>18)&0b00111111);
			utf[2] = 0b10000000|((unicode>>12)&0b00111111);
			utf[3] = 0b10000000|((unicode>>6)&0b00111111);
			utf[4] = 0b10000000|(unicode&0b00111111);
		} else {
			utf_len = 6;
			utf[0] = 0b11110000|((unicode>>30)&0b00000001);
			utf[1] = 0b10000000|((unicode>>24)&0b00111111);
			utf[2] = 0b10000000|((unicode>>18)&0b00111111);
			utf[3] = 0b10000000|((unicode>>12)&0b00111111);
			utf[4] = 0b10000000|((unicode>>6)&0b00111111);
			utf[5] = 0b10000000|(unicode&0b00111111);
		}
		text->replace(i, 10, utf, utf_len);
		i += utf_len;
		p = text->c_str();
		len = text->size();
	}
	return text;
}
} //namespace ugc_emoji
