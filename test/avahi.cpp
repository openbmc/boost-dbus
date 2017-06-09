// Copyright (c) Benjamin Kietzman (github.com/bkietz)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <dbus/connection.hpp>
#include <dbus/endpoint.hpp>
#include <dbus/filter.hpp>
#include <dbus/match.hpp>
#include <dbus/message.hpp>
#include <dbus/utility.hpp>
#include <functional>

#include <unistd.h>
#include <gmock/gmock.h>
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

TEST(BOOST_DBUS, ListServices) {
  boost::asio::io_service io;
  boost::asio::deadline_timer t(io, boost::posix_time::seconds(10));
  t.async_wait([&](const boost::system::error_code& /*e*/) {
    io.stop();
    FAIL() << "Callback was never called\n";
  });

  dbus::connection system_bus(io, dbus::bus::system);

  dbus::endpoint test_daemon("org.freedesktop.DBus", "/",
                             "org.freedesktop.DBus");
  // create new service browser
  dbus::message m = dbus::message::new_call(test_daemon, "ListNames");
  system_bus.async_send(
      m, [&](const boost::system::error_code ec, dbus::message r) {
        io.stop();
        std::vector<std::string> services;
        r.unpack(services);
        // Test a couple things that should always be present.... adapt if
        // neccesary
        EXPECT_THAT(services, testing::Contains("org.freedesktop.DBus"));
        EXPECT_THAT(services, testing::Contains("org.freedesktop.Accounts"));

      });

  io.run();
}

TEST(BOOST_DBUS, ListObjects) {
  boost::asio::io_service io;
  dbus::connection system_bus(io, dbus::bus::system);

  dbus::endpoint test_daemon("org.freedesktop.DBus", "/",
                             "org.freedesktop.DBus");

  // create new service browser
  dbus::message m = dbus::message::new_call(test_daemon, "ListNames");
  system_bus.async_send(m, [&](const boost::system::error_code ec,
                               dbus::message r) {
    static std::vector<std::string> services;
    r.unpack(services);
    // todo(ed) find out why this needs to be atomic
    static std::atomic<int> dbus_count(0);
    std::cout << dbus_count << " Callers\n";
    auto names = std::make_shared<std::vector<std::string>>();
    for (auto& service : services) {
      dbus::endpoint service_daemon(service, "/",
                                    "org.freedesktop.DBus.Introspectable");
      dbus::message m = dbus::message::new_call(service_daemon, "Introspect");
      dbus_count++;
      std::cout << dbus_count << " Callers\n";

      system_bus.async_send(m, [&io, &service, names](
                                   const boost::system::error_code ec,
                                   dbus::message r) {
        dbus_count--;
        std::cout << service << "\n";
        // Todo(ed) figure out why we're occassionally getting access denied
        // errors
        // EXPECT_EQ(ec, boost::system::errc::success);
        if (ec) {
          std::cout << "Error:" << ec << " reading service " << service << "\n";
        } else {
          std::string xml;
          r.unpack(xml);
          size_t old_size = names->size();
          // TODO(ed) names needs lock for multithreaded access
          dbus::read_dbus_xml_names(xml, *names);
          // loop over the newly added items
          for (size_t name_index = old_size; name_index < names->size();
               name_index++) {
            // auto& name = names[name_index];
          }
        }
        // if we're the last one, print the list and cancel the io_service
        if (dbus_count == 0) {
          for (auto& name : *names) {
            std::cout << name << "\n";
          }
          io.stop();
        }
      });

      std::function<void(const boost::system::error_code ec, dbus::message r)>
          event_handler =
              [&](const boost::system::error_code ec, dbus::message r) {};
      system_bus.async_send(m, event_handler);
    }

  });

  io.run();
}
