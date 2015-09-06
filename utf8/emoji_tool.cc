#include "emoji_mysql_fix.h"

#include <string>
#include <iostream>

using namespace ugc_emoji;

// for testing purpose only
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

#define IS_HEX_DIGIT(c) ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))

size_t get_partial_escape(const char *line, size_t len, char *buf) {
	auto p = line + len - 1;
	size_t count = 0;
	while (IS_HEX_DIGIT(*p) && count < 8 && p >= line) {
		++count;
		--p;
	}
	if (count >= 8) {
		return 0;
	}
	if (p >= line) {
		if (*p == '\\') {
			++count;
		} else if (p > line && *p == 'U' && *(--p) == '\\') {
			count += 2;
		} else {
			count = 0;
		}
	} else {
		++p;
	}
	for (unsigned i = 0; i < count; ++i) {
		buf[i] = p[i];
	}
	return count;
	
}

size_t get_partial_utf8(const char *line, size_t len, char *buf) {
	auto p = line + len - 1;
	if ((*p & 0b11000000) != 0b10000000 && (*p & 0b11000000) != 0b11000000) {
		return 0;
	}
	size_t count = 0;
	while ((*p & 0b11000000) == 0b10000000 && count < 6 && p >= line) {
		++count;
		--p;
	}
	if (count >= 6) {
		return 0;
	}
	if (p >= line) {
		++count;
		if ((*p & 0b11100000) == 0b11000000) {
			if (count == 3) count = 0;
		} else if ((*p & 0b11110000) == 0b11100000) {
			if (count == 4) count = 0;
		} else if ((*p & 0b11111000) == 0b11110000) {
			if (count == 5) count = 0;
		} else if ((*p & 0b11111100) == 0b11111000) {
			if (count == 6) count = 0;
		} else if ((*p & 0b11111110) == 0b11111100) {
			if (count == 7) count = 0;
		} else {
			count = 0;
		}
	} else {
		++p;
	}
	for (unsigned i = 0; i < count; ++i) {
		buf[i] = p[i];
	}
	return count;
	
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
	auto partial_func = [=] {
		std::string name(argv[0]);
		return name.find("decode") == std::string::npos ? get_partial_utf8 : get_partial_escape;
	}();
	std::string line;
	char buf[16];
	size_t buf_len = 0;
	auto err_mask = (std::ios_base::eofbit|std::ios_base::failbit|std::ios_base::badbit);
	for ( ; !(std::cin.rdstate()&err_mask); ) {
		line.resize(1024);
		if (buf_len > 0) {
			line.replace(0, buf_len, buf, buf_len);
		}
		std::cin.read(&*line.begin() + buf_len, line.size() - buf_len - 1);
		auto len = std::cin.gcount();
		if (len > 0) {
			len += buf_len;
			buf_len = partial_func(&*line.begin(), len, buf);
			len -= buf_len;
			line.resize(len);
			std::cout << *func(&line);
		}
	}
	if (buf_len > 0) {
		buf[buf_len] = 0;
		std::cout << buf;
	}
}

