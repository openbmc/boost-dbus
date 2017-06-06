// Copyright (c) Benjamin Kietzman (github.com/bkietz)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <dbus/connection.hpp>
#include <dbus/endpoint.hpp>
#include <dbus/filter.hpp>
#include <dbus/match.hpp>
#include <dbus/message.hpp>
#include <functional>

#include <unistd.h>
#include <gtest/gtest.h>

TEST(AvahiTest, GetHostName) {
  dbus::endpoint test_daemon("org.freedesktop.Avahi", "/",
                             "org.freedesktop.Avahi.Server");
  boost::asio::io_service io;
  dbus::connection system_bus(io, dbus::bus::system);

  dbus::message m = dbus::message::new_call(test_daemon, "GetHostName");

  system_bus.async_send(
      m, [&](const boost::system::error_code ec, dbus::message r) {

        std::string avahi_hostname;
        std::string hostname;

        // get hostname from a system call
        char c[1024];
        gethostname(c, 1024);
        hostname = c;

        r.unpack(avahi_hostname);

        // Get only the host name, not the fqdn
        auto unix_hostname = hostname.substr(0, hostname.find("."));
        EXPECT_EQ(unix_hostname, avahi_hostname);

        io.stop();
      });
  boost::asio::deadline_timer t(io, boost::posix_time::seconds(10));
  t.async_wait([&](const boost::system::error_code& /*e*/) {
    io.stop();
    FAIL() << "Callback was never called\n";
  });
  io.run();
}

TEST(AvahiTest, ServiceBrowser) {
  boost::asio::io_service io;
  dbus::connection system_bus(io, dbus::bus::system);

  dbus::endpoint test_daemon("org.freedesktop.Avahi", "/",
                             "org.freedesktop.Avahi.Server");
  // create new service browser
  dbus::message m1 = dbus::message::new_call(test_daemon, "ServiceBrowserNew");
  m1.pack<int32_t>(-1)
   .pack<int32_t>(-1)
   .pack<std::string>("_http._tcp")
   .pack<std::string>("local")
   .pack<uint32_t>(0);

  dbus::message r = system_bus.send(m1);
  std::string browser_path;
  r.unpack(browser_path);
  testing::Test::RecordProperty("browserPath", browser_path);

  dbus::match ma(system_bus, "type='signal',path='" + browser_path + "'");
  dbus::filter f(system_bus, [](dbus::message& m) {
    auto member = m.get_member();
    return member == "NameAcquired";
  });

  std::function<void(boost::system::error_code, dbus::message)> event_handler =
      [&](boost::system::error_code ec, dbus::message s) {
        testing::Test::RecordProperty("firstSignal", s.get_member());
        std::string a = s.get_member();
        std::string dude;
        s.unpack(dude);
        f.async_dispatch(event_handler);
        io.stop();
      };
  f.async_dispatch(event_handler);

  boost::asio::deadline_timer t(io, boost::posix_time::seconds(10));
  t.async_wait([&](const boost::system::error_code& /*e*/) {
    io.stop();
    FAIL() << "Callback was never called\n";
  });
  io.run();
}
