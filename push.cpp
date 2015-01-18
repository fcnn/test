//
// client.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2014 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/thread.hpp>

enum { max_length = 1024 };

void token2bytes(const char *token, char *bytes) {
  for (int i = 0; *token && i < 32; ++i) {
    char val = 0;
    if (*token >= 'A' && *token <= 'F') {
      val = (*token - 'A' + 10) << 4;
    } else if (*token >= 'a' && *token <= 'f') {
      val = (*token - 'a' + 10) << 4;
    } else {
      val = (*token - '0') << 4;
    }
    ++token;
    if (*token >= 'A' && *token <= 'F') {
      val += *token - 'A' + 10;
    } else if (*token >= 'a' && *token <= 'f') {
      val += *token - 'a' + 10;
    } else {
      val += *token - '0';
    }
    *bytes++ = val;
    ++token;
    while (*token == ' ') {
      ++token;
    }
  }
}

class client
{
public:
  int m_num;
  char message_[1024 * 5];
  bool PushMsg(const std::string& msg, const std::string& token, int32_t max_msg_len) {
    bool ret = true;
    char *pointer = message_;
    *pointer++ = 2;
    pointer += 4;
    
    *pointer++ = 1;
    uint16_t len = htons(32);
    memcpy(pointer, &len, sizeof (len));
    pointer += sizeof (len);
    token2bytes(token.c_str(), pointer);
    pointer += 32;
    
    *pointer++ = 2;
    size_t msglen = msg.length();
    if (msglen > max_msg_len - 48) {
      msglen = max_msg_len - 48;
    }
    len = htons(msglen + 48);
    memcpy(pointer, &len, sizeof (len));
    pointer += sizeof (len);
    memcpy(pointer, "{\"aps\":{\"alert\":\"", 17);
    pointer += 17;
    memcpy(pointer, msg.c_str(), msglen);
    pointer += msglen;
    memcpy(pointer, "\",\"sound\":\"default\",\"badge\":1}}", 31);
    pointer += 31;

    *pointer++ = 3;
    len = htons(4);
    memcpy(pointer, &len, sizeof (len));
    pointer += sizeof (len);
    static uint32_t _id = 0;
    uint32_t u32 = htonl(++_id);
    memcpy(pointer, &u32, sizeof (u32));
    pointer += sizeof (u32);
    
    *pointer++ = 4;
    len = htons(4);
    memcpy(pointer, &len, sizeof (len));
    pointer += sizeof (len);
    u32 = htonl(time(NULL) + 3600 * 24);
    memcpy(pointer, &u32, sizeof (u32));
    pointer += sizeof (u32);
    
    *pointer++ = 5;
    len = htons(1);
    memcpy(pointer, &len, sizeof (len));
    pointer += sizeof (len);
    *pointer++ = 10;

    int length_ = static_cast<int>(pointer - message_);

    u32 = htonl(length_ - 5);
    memcpy(message_ + 1, &u32, sizeof (u32));
    
    memcpy(message_ + length_, message_, length_);
    message_[length_ + 60] = '^';
    message_[length_ + 61] = '_';
    message_[length_ + 62] = '^';
    length_ *= 2;
    u32 = htonl(++_id);
    memcpy(message_ + length_ - 15, &u32, sizeof (u32));

    std::cout << __FUNCTION__ << ": bytes to send = " << length_ << std::endl;
    boost::asio::async_write(socket_,
    boost::asio::buffer(message_, length_),
    boost::bind(&client::handle_write, this,
    boost::asio::placeholders::error,
    boost::asio::placeholders::bytes_transferred));
    std::cout << __FUNCTION__ << ": sent" << std::endl;
  }

  bool PushMsgOld(const std::string& msg, const std::string& token, int32_t max_msg_len) {
    bool ret = true;
    char message_[4096];
    char *pointer = message_;
    *pointer++ = 0;
    uint16_t len = htons(32);
    memcpy(pointer, &len, sizeof (len));
    pointer += sizeof (len);
    token2bytes(token.c_str(), pointer);
    pointer += 32;
    size_t msglen = msg.length();
    if (msglen > max_msg_len - 48) {
      msglen = max_msg_len - 48;
    }
    len = htons(msglen + 48);
    memcpy(pointer, &len, sizeof (len));
    pointer += sizeof (len);

    memcpy(pointer, "{\"aps\":{\"alert\":\"", 17);
    pointer += 17;
    memcpy(pointer, msg.c_str(), msglen);
    pointer += msglen;
    memcpy(pointer, "\",\"sound\":\"default\",\"badge\":1}}", 31);
    pointer += 31;

    int length_ = static_cast<int>(pointer - message_);
    std::cout << __FUNCTION__ << ": bytes to send = " << length_ << std::endl;
    boost::asio::async_write(socket_,
    boost::asio::buffer(message_, length_),
    boost::bind(&client::handle_write, this,
    boost::asio::placeholders::error,
    boost::asio::placeholders::bytes_transferred));
    std::cout << __FUNCTION__ << ": sent" << std::endl;
  }

  bool PushMsg_IOS7_Below(const std::string& msg, const std::string& token) {
    return PushMsg(msg, token, 256);
  }

  bool PushMsg_IOS8_Above(const std::string& msg, const std::string& token) {
    return PushMsg(msg, token, 2048);
  }

  client(boost::asio::io_service& io_service,
      boost::asio::ssl::context& context,
      boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
    : socket_(io_service, context)
  {
    m_num = 0;
    socket_.set_verify_mode(boost::asio::ssl::verify_peer);
    socket_.set_verify_callback(
        boost::bind(&client::verify_certificate, this, _1, _2));

    boost::asio::async_connect(socket_.lowest_layer(), endpoint_iterator,
        boost::bind(&client::handle_connect, this,
          boost::asio::placeholders::error));
  }

  bool verify_certificate(bool preverified,
      boost::asio::ssl::verify_context& ctx)
  {
    // The verify callback can be used to check whether the certificate that is
    // being presented is valid for the peer. For example, RFC 2818 describes
    // the steps involved in doing this for HTTPS. Consult the OpenSSL
    // documentation for more details. Note that the callback is called once
    // for each certificate in the certificate chain, starting from the root
    // certificate authority.

    // In this example we will simply print the certificate's subject name.
    char subject_name[512];
    X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 512);
    std::cout << "Verifying " << subject_name << " preverified = " << preverified <<"\n";

    // return preverified;
    return true;
  }

  void handle_connect(const boost::system::error_code& error)
  {
    if (!error)
    {
      socket_.async_handshake(boost::asio::ssl::stream_base::client,
          boost::bind(&client::handle_handshake, this,
            boost::asio::placeholders::error));
    }
    else
    {
      std::cout << "Connect failed: " << error.message() << "\n";
    }
  }

  void handle_handshake(const boost::system::error_code& error)
  {
    if (!error)
    {
      char reply_[256];
      boost::asio::async_read(socket_,
          boost::asio::buffer(reply_, 256),
          boost::bind(&client::handle_read, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
      const char *token = "1427600ea1883f664d2bff04a855273525b7e61090118280e1e36a9fc882de58";
      const char *msg2 = "hehe1";
      const char *msg = "hello world! hello'' i...world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello worrld! hello world! hello world! hello'' i...world! hello world! hello world! hello world! hello rld! hello world! hello world! hello'' i...world! hello world! hello world! hello world! hello rld! hello world! hello world! hello'' i...world! hello world! hello world! hello world! hello ld! hello world! hello world! hello'' i...world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello'' i...world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello'' i...world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! wait ... IOS8 can see this:)";
      PushMsg_IOS8_Above(msg, token);
      // boost::this_thread::sleep(boost::posix_time::seconds(60 * 10));
    }
    else
    {
      std::cout << "Handshake failed: " << error.message() << "\n";
    }
  }

  void handle_write(const boost::system::error_code& error,
      size_t bytes_transferred)
  {
    if (!error)
    {
      std::cout << __FUNCTION__ << " bytes_transferred = " << bytes_transferred << std::endl;
      if (++m_num >= 5) {
        return;
      }
      const char *token = "1427600ea1883f664d2bff04a855273525b7e61090118280e1e36a9fc882de58";
      const char *msg2 = ":) hehe1";
      const char *msg = ":) hello world! hello'' i...world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello worrld! hello world! hello world! hello'' i...world! hello world! hello world! hello world! hello rld! hello world! hello world! hello'' i...world! hello world! hello world! hello world! hello rld! hello world! hello world! hello'' i...world! hello world! hello world! hello world! hello ld! hello world! hello world! hello'' i...world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello'' i...world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello'' i...world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! hello world! wait ... IOS8 can see this:)";
      PushMsg_IOS8_Above(msg2, token);
      // boost::this_thread::sleep(boost::posix_time::seconds(60 * 10));
#if 0
      boost::asio::async_read(socket_,
          boost::asio::buffer(reply_, bytes_transferred),
          boost::bind(&client::handle_read, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
#endif // 0
    }
    else
    {
      std::cout << "Write failed: " << error.message() << "\n";
    }
  }

  void handle_read(const boost::system::error_code& error,
      size_t bytes_transferred)
  {
    if (!error)
    {
      std::cout << "Reply: ";
      std::cout.write(reply_, bytes_transferred);
      std::cout << "\n";
    }
    else
    {
      std::cout << "Read failed: " << error.message() << "\n";
    }
  }

private:
  boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket_;
  char request_[max_length];
  char reply_[max_length];
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: client <host> <port>\n";
      return 1;
    }

    boost::asio::io_service io_service;

    boost::asio::ip::tcp::resolver resolver(io_service);
    boost::asio::ip::tcp::resolver::query query(argv[1], argv[2]);
    boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);

    boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv23);
    ctx.load_verify_file("push.pem");
    boost::system::error_code err;
    ctx.use_certificate_chain_file("push.pem", err);
    if (err) {
      std::cerr << "err chain file" << std::endl;
      exit (1);
    }
    ctx.use_private_key_file("push.pem",boost::asio::ssl::context::pem);
    ctx.use_certificate_file("push.pem",boost::asio::ssl::context::pem);

    client c(io_service, ctx, iterator);

    io_service.run();
    std::cout << "exit ..." << std::endl;
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }


  return 0;
}
