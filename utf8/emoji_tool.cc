#include "emoji_mysql_fix.h"

#include <string>
#include <iostream>

using namespace ugc_emoji;

int decode_utf8(const char *msg) {
	for (auto p = msg; *p; ) {
		unsigned char c = *p;
		unsigned char buf[7];
		buf[0] = c;
		++p;
		if (c < 0b10000000) {
			buf[1] = 0;
		} else if (c < 0b11100000) {
			if (c > 0b11000000) {
				if (*p != 0) {
					buf[1] = *p++;
					buf[2] = 0;
				} else {
					buf[1] = 0;
				}
			} else {
				buf[1] = 0;
			}
		} else if (c < 0b11110000) {
			if (*p != 0) {
				buf[1] = *p++;
				if (*p != 0) {
					buf[2] = *p++;
					buf[3] = 0;
				} else {
					buf[2] = 0;
				}
			} else {
				buf[1] = 0;
			}
		} else if (c < 0b11111000) {
			if (*p != 0) {
				buf[1] = *p++;
				if (*p != 0) {
					buf[2] = *p++;
					if (*p != 0) {
						buf[3] = *p++;
						buf[4] = 0;
					} else {
						buf[3] = 0;
					}
				} else {
					buf[2] = 0;
				}
			} else {
				buf[1] = 0;
			}
		} else if (c < 0b11111100) {
			if (*p != 0) {
				buf[1] = *p++;
				if (*p != 0) {
					buf[2] = *p++;
					if (*p != 0) {
						buf[3] = *p++;
						if (*p != 0) {
							buf[4] = *p++;
							buf[5] = 0;
						} else {
							buf[4] = 0;
						}
					} else {
						buf[3] = 0;
					}
				} else {
					buf[2] = 0;
				}
			} else {
				buf[1] = 0;
			}
		} else if (c < 0b11111110) {
			if (*p != 0) {
				buf[1] = *p++;
				if (*p != 0) {
					buf[2] = *p++;
					if (*p != 0) {
						buf[3] = *p++;
						if (*p != 0) {
							buf[4] = *p++;
							if (*p != 0) {
								buf[5] = *p++;
								buf[6] = 0;
							} else {
								buf[5] = 0;
							}
						} else {
							buf[4] = 0;
						}
					} else {
						buf[3] = 0;
					}
				} else {
					buf[2] = 0;
				}
			} else {
				buf[1] = 0;
			}
		} else {
			buf[1] = 0;
		}
		std::cout << buf;
	}
	std::cout << "\n";
	return 0;
}

int main(int argc, char *argv[]) {
	// std::string msg("Emoji测试。✊ 🐂 🌹 u ☝🏻 👏🏻 🌻 👊🏻");
	// decode_utf8(msg.c_str());
	// std::cout << *encode_emoji(&msg) << "\n";
	// std::cout << *decode_emoji(&msg) << "\n";
	auto func = [=] {
		std::string name(argv[0]);
		return name.find("decode") == std::string::npos ? encode_emoji : decode_emoji;
	}();
	std::string line;
	char buf[10];
	size_t buf_len = 0;
	auto err_mask = (std::ios_base::eofbit|std::ios_base::failbit|std::ios_base::badbit);
	for ( ; !(std::cin.rdstate()&err_mask); ) {
		line.resize(20);
		if (buf_len > 0) {
			line.replace(
		}
		std::cin.read(&*line.begin(), line.size() - 1);
		auto len = std::cin.gcount();
		if (len > 0) {
			line.resize(len);
			std::cout << *func(&line);
		}
	}
}

