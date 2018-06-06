/* nsemu - LGPL - Copyright 2018 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"
#include "IpcStubs.hpp"

uint32_t nn::fssrv::sf::IFileSystemProxy::OpenBisPartition(nn::fssrv::sf::Partition partitionID, nn::fssrv::sf::IStorage* BisPartition) {
	ns_print("Stub implementation for nn::fssrv::sf::IFileSystemProxy::OpenBisPartition\n");
	//BisPartition = buildInterface(nn::fssrv::sf::IStorage, "bis.istorage");
	return 0x0;
}

uint32_t nn::fssrv::sf::IFileSystemProxy::OpenDataStorageByApplicationId(nn::ApplicationId tid, nn::fssrv::sf::IStorage* dataStorage) {
	ns_print("Stub implementation for nn::fssrv::sf::IFileSystemProxy::OpenDataStorageByApplicationId 0x%lx\n", tid);
	std::stringstream ss;
	ss << "tid_archives_" << hex << tid << ".istorage";
	//dataStorage = buildInterface(nn::fssrv::sf::IStorage, ss.str());
	return 0;
}

uint32_t nn::fssrv::sf::IFileSystemProxy::OpenDataStorageByCurrentProcess(nn::fssrv::sf::IStorage* dataStorage) {
	ns_print("Stub implementation for nn::fssrv::sf::IFileSystemProxy::OpenDataStorageByCurrentProcess\n");
	//dataStorage = buildInterface(nn::fssrv::sf::IStorage, "");
	return 0;
}

uint32_t nn::fssrv::sf::IFileSystemProxy::OpenDataStorageByDataId(nn::ApplicationId tid, uint8_t storageId, nn::fssrv::sf::IStorage* dataStorage) {
	ns_print("Stub implementation for nn::fssrv::sf::IFileSystemProxy::OpenDataStorageByDataId 0x%lx\n", 0x0100000000000800+(uint64_t)storageId);
	std::stringstream ss;
	ss << "archives/" << hex << setw(16) << setfill('0') << 0x0100000000000800+(uint64_t)storageId << ".istorage";
	//dataStorage = buildInterface(nn::fssrv::sf::IStorage, ss.str());
	return 0;
}

uint32_t nn::fssrv::sf::IFileSystemProxy::OpenGameCardPartition(nn::fssrv::sf::Partition partitionID, uint32_t _1, nn::fssrv::sf::IStorage* gameCardFs) {
	ns_print("Stub implementation for nn::fssrv::sf::IFileSystemProxy::OpenGameCardPartition\n");
	//gameCardFs = buildInterface(nn::fssrv::sf::IStorage, "GamePartition.istorage");
	return 0;
}

uint32_t nn::fssrv::sf::IFileSystemProxy::OpenRomStorage(nn::fssrv::sf::IStorage* _0) {
	ns_print("Stub implementation for nn::fssrv::sf::IFileSystemProxy::OpenRomStorage\n");
	//_0 = buildInterface(nn::fssrv::sf::IStorage, "RomStorage.istorage");
	return 0;
}
