/* nsemu - LGPL - Copyright 2018 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"
#include "IpcStubs.hpp"

uint32_t nn::socket::sf::IClient::Accept(uint32_t socket, int32_t& ret, uint32_t& bsd_errno, uint32_t& sockaddr_len, sockaddr *_addr, unsigned _size) {
	ns_print("nn::socket::sf::IClient::accept\n");
	struct sockaddr *addr = (struct sockaddr *) _addr;
	socklen_t size = (uint32_t) _size;
	ret = ::accept(socket, addr, &size);
	bsd_errno = errno;
	sockaddr_len = size;
	addr->sa_family = htons(addr->sa_family);
	return 0;
}

uint32_t nn::socket::sf::IClient::Bind(uint32_t socket, sockaddr * _addr, unsigned _size, int32_t& ret, uint32_t& bsd_errno) {
	ns_print("nn::socket::sf::IClient::bind\n");
	struct sockaddr *addr = (struct sockaddr *) _addr;
	addr->sa_family = ntohs(addr->sa_family);
	ret = ::bind(socket, addr, (uint32_t) _size);
	bsd_errno = errno;
	return 0;
}

uint32_t nn::socket::sf::IClient::Close(uint32_t socket, int32_t& ret, uint32_t& bsd_errno) {
	ns_print("nn::socket::sf::IClient::bsd_close\n");
	ret = ::close(socket);
	bsd_errno = errno;
	return 0;
}

uint32_t nn::socket::sf::IClient::Connect(uint32_t socket, sockaddr * _addr, unsigned _size, int32_t& ret, uint32_t& bsd_errno) {
	ns_print("nn::socket::sf::IClient::connect\n");
	struct sockaddr *addr = (struct sockaddr *) _addr;
	addr->sa_family = ntohs(addr->sa_family); // yes, this is network byte order on the switch and host byte order on linux
	ret = ::connect(socket, (struct sockaddr *) addr, (socklen_t) _size);
	bsd_errno = errno;
	return 0;
}
uint32_t nn::socket::sf::IClient::GetSockName(uint32_t socket, int32_t& ret, uint32_t& bsd_errno, uint32_t& sockaddr_len, sockaddr * _addr, unsigned _size) {
	ns_print("nn::socket::sf::IClient::getsockname\n");

	struct sockaddr *addr = (struct sockaddr *) _addr;
	socklen_t addr_len = (socklen_t) _size;
	ret = ::getsockname(socket, addr, &addr_len);
	errno = bsd_errno;
	sockaddr_len = addr_len;
	addr->sa_family = htons(addr->sa_family);
	return 0;
}

uint32_t nn::socket::sf::IClient::Listen(uint32_t socket, uint32_t backlog, int32_t& ret, uint32_t& bsd_errno) {
	ns_print("nn::socket::sf::IClient::listen\n");
	ret = ::listen(socket, backlog);
	bsd_errno = errno;
	return 0;
}

uint32_t nn::socket::sf::IClient::Recv(uint32_t socket, uint32_t flags, int32_t& ret, uint32_t& bsd_errno, int8_t * buf, unsigned _size) {
	ns_print("nn::socket::sf::IClient::recv\n");

	ret = (int32_t) ::recv(socket, buf, _size, flags);
	bsd_errno = errno;
	return 0;
}

uint32_t nn::socket::sf::IClient::Send(uint32_t socket, uint32_t flags, int8_t * buf, unsigned _size, int32_t& ret, uint32_t& bsd_errno) {
	ns_print("nn::socket::sf::IClient::send\n");

	ret = (uint32_t) ::send(socket, buf, (size_t) _size, flags);
	bsd_errno = errno;
	return 0;
}

uint32_t nn::socket::sf::IClient::SendTo(uint32_t socket, uint32_t flags, int8_t * buf, unsigned _size, sockaddr * _addr, unsigned _addr_size, int32_t& ret, uint32_t& bsd_errno) {
	ns_print("nn::socket::sf::IClient::sendto\n");

	struct sockaddr *addr = (struct sockaddr *) _addr;
	addr->sa_family = ntohs(addr->sa_family);
	ret = (uint32_t) ::sendto(socket, buf, (size_t) _size, flags, (struct sockaddr *) addr, (socklen_t) _addr_size);
	bsd_errno = errno;
	return 0;
}

uint32_t nn::socket::sf::IClient::SetSockOpt(uint32_t socket, uint32_t level, uint32_t option_name, uint8_t * optval, unsigned _opt_size, int32_t& ret, uint32_t& bsd_errno) {
	ns_print("nn::socket::sf::IClient::setsockopt\n");
	return 0;
}

uint32_t nn::socket::sf::IClient::Shutdown(uint32_t socket, uint32_t how, int32_t& ret, uint32_t& bsd_errno) {
	ns_print("nn::socket::sf::IClient::shutdown");
	ret = ::shutdown(socket, how);
	return 0;
}
uint32_t nn::socket::sf::IClient::Socket(uint32_t domain, uint32_t type, uint32_t protocol, int32_t& ret, uint32_t& bsd_errno) {
	ns_print("nn::socket::sf::IClient::socket");

	ret = ::socket(domain, type, protocol);
	bsd_errno = errno;
	return 0;
}
