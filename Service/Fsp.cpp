/* nsemu - LGPL - Copyright 2018 rkx1209<rkx1209dev@gmail.com> */
#include "Nsemu.hpp"
#include "IpcStubs.hpp"

uint32_t nn::fssrv::sf::IFileSystemProxy::OpenBisPartition(nn::fssrv::sf::Partition partitionID, nn::fssrv::sf::IStorage*& BisPartition) {
	ns_print("Stub implementation for nn::fssrv::sf::IFileSystemProxy::OpenBisPartition\n");
	BisPartition = new nn::fssrv::sf::IStorage();
	return 0x0;
}

uint32_t nn::fssrv::sf::IFileSystemProxy::OpenDataStorageByApplicationId(nn::ApplicationId tid, nn::fssrv::sf::IStorage*& dataStorage) {
	ns_print("Stub implementation for nn::fssrv::sf::IFileSystemProxy::OpenDataStorageByApplicationId 0x%lx\n", tid);
	std::stringstream ss;
	ss << "tid_archives_" << hex << tid << ".istorage";
	dataStorage = new nn::fssrv::sf::IStorage();
	return 0;
}

uint32_t nn::fssrv::sf::IFileSystemProxy::OpenDataStorageByCurrentProcess(nn::fssrv::sf::IStorage*& dataStorage) {
	ns_print("Stub implementation for nn::fssrv::sf::IFileSystemProxy::OpenDataStorageByCurrentProcess\n");
	dataStorage = new nn::fssrv::sf::IStorage();
	return 0;
}

uint32_t nn::fssrv::sf::IFileSystemProxy::OpenDataStorageByDataId(nn::ApplicationId tid, uint8_t storageId, nn::fssrv::sf::IStorage*& dataStorage) {
	ns_print("Stub implementation for nn::fssrv::sf::IFileSystemProxy::OpenDataStorageByDataId 0x%lx\n", 0x0100000000000800+(uint64_t)storageId);
	std::stringstream ss;
	ss << "archives/" << hex << setw(16) << setfill('0') << 0x0100000000000800+(uint64_t)storageId << ".istorage";
	dataStorage = new nn::fssrv::sf::IStorage();
	return 0;
}

uint32_t nn::fssrv::sf::IFileSystemProxy::OpenGameCardPartition(nn::fssrv::sf::Partition partitionID, uint32_t _1, nn::fssrv::sf::IStorage*& gameCardFs) {
	ns_print("Stub implementation for nn::fssrv::sf::IFileSystemProxy::OpenGameCardPartition\n");
	gameCardFs = new nn::fssrv::sf::IStorage();
	return 0;
}

uint32_t nn::fssrv::sf::IFileSystemProxy::OpenRomStorage(nn::fssrv::sf::IStorage*& _0) {
	ns_print("Stub implementation for nn::fssrv::sf::IFileSystemProxy::OpenRomStorage\n");
	_0 = new nn::fssrv::sf::IStorage();
	return 0;
}

// Funcs
uint32_t nn::fssrv::sf::IFileSystemProxy::OpenDataFileSystemByCurrentProcess(nn::fssrv::sf::IFileSystem*& _0) {
	ns_print("Stub implementation for nn::fssrv::sf::IFileSystemProxy::OpenDataFileSystemByCurrentProcess");
	_0 = new nn::fssrv::sf::IFileSystem();
	return 0;
}

uint32_t nn::fssrv::sf::IFileSystemProxy::MountContent7(nn::ApplicationId tid, uint32_t ncaType, nn::fssrv::sf::IFileSystem*& _2) {
	ns_print("Stub implementation for nn::fssrv::sf::IFileSystemProxy::MountContent7");
	_2 = new nn::fssrv::sf::IFileSystem();
	return 0;
}

uint32_t nn::fssrv::sf::IFileSystemProxy::MountContent(nn::ApplicationId tid, uint32_t flag, int8_t * path, unsigned int path_size, nn::fssrv::sf::IFileSystem*& contentFs) {
	ns_print("Stub implementation for nn::fssrv::sf::IFileSystemProxy::MountContent");
	contentFs = new nn::fssrv::sf::IFileSystem();
	return 0;
}

uint32_t nn::fssrv::sf::IFileSystemProxy::OpenDataFileSystemByApplicationId(nn::ApplicationId tid, nn::fssrv::sf::IFileSystem*& dataFiles) {
	ns_print("Stub implementation for nn::fssrv::sf::IFileSystemProxy::OpenDataFileSystemByApplicationId");
	dataFiles = new nn::fssrv::sf::IFileSystem();
	return 0;
}

uint32_t nn::fssrv::sf::IFileSystemProxy::MountBis(nn::fssrv::sf::Partition partitionID, int8_t * path, unsigned int path_size, nn::fssrv::sf::IFileSystem*& Bis) {
	ns_print("Stub implementation for nn::fssrv::sf::IFileSystemProxy::MountBis");
	Bis = new nn::fssrv::sf::IFileSystem();
	return 0;
}

uint32_t nn::fssrv::sf::IFileSystemProxy::OpenHostFileSystemImpl(int8_t * path, unsigned int path_size, nn::fssrv::sf::IFileSystem*& _1) {
	ns_print("Stub implementation for nn::fssrv::sf::IFileSystemProxy::OpenHostFileSystemImpl");
	_1 = new nn::fssrv::sf::IFileSystem();
	return 0;
}

uint32_t nn::fssrv::sf::IFileSystemProxy::MountSdCard(nn::fssrv::sf::IFileSystem*& sdCard) {
	ns_print("Stub implementation for nn::fssrv::sf::IFileSystemProxy::MountSdCard");
	//sdCard = new nn::fssrv::sf::IFileSystem, "SDCard");
        sdCard = new nn::fssrv::sf::IFileSystem();
	return 0;
}

uint32_t nn::fssrv::sf::IFileSystemProxy::MountGameCardPartition(uint32_t _0, uint32_t _1, nn::fssrv::sf::IFileSystem*& gameCardPartitionFs) {
	ns_print("Stub implementation for nn::fssrv::sf::IFileSystemProxy::MountGameCardPartition");
	//gameCardPartitionFs = new nn::fssrv::sf::IFileSystem, "GameCard");
        gameCardPartitionFs = new nn::fssrv::sf::IFileSystem();
	return 0;
}

uint32_t nn::fssrv::sf::IFileSystemProxy::MountSaveData(uint8_t input, nn::fssrv::sf::SaveStruct saveStruct, nn::fssrv::sf::IFileSystem*& saveDataFs) {
	uint64_t tid = *(uint64_t *)(&saveStruct[0x18]);
	ns_print("Stub implementation for nn::fssrv::sf::IFileSystemProxy::MountSaveData 0x%lx", tid);
	std::stringstream ss;
	ss << "save_" << hex << tid;
	//saveDataFs = new nn::fssrv::sf::IFileSystem, ss.str());
        saveDataFs = new nn::fssrv::sf::IFileSystem();
	return 0;
}

uint32_t nn::fssrv::sf::IFileSystemProxy::MountSystemSaveData(uint8_t input, nn::fssrv::sf::SaveStruct saveStruct, nn::fssrv::sf::IFileSystem*& systemSaveDataFs) {
	uint64_t tid = *(uint64_t *)(&saveStruct[0x18]);
	ns_print("Stub implementation for nn::fssrv::sf::IFileSystemProxy::MountSystemSaveData 0x%lx", tid);
	std::stringstream ss;
	ss << "syssave_" << hex << tid;
	//systemSaveDataFs = new nn::fssrv::sf::IFileSystem, ss.str());
        systemSaveDataFs = new nn::fssrv::sf::IFileSystem();
	return 0;
}

uint32_t nn::fssrv::sf::IFileSystemProxy::MountSaveDataReadOnly(uint8_t input, nn::fssrv::sf::SaveStruct saveStruct, nn::fssrv::sf::IFileSystem*& saveDataFs) {
	uint64_t tid = *(uint64_t *)(&saveStruct[0x18]);
	ns_print("Stub implementation for nn::fssrv::sf::IFileSystemProxy::MountSaveDataReadOnly 0x%lx", tid);
	std::stringstream ss;
	ss << "save_" << hex << tid;
	//saveDataFs = new nn::fssrv::sf::IFileSystem, ss.str());
        saveDataFs = new nn::fssrv::sf::IFileSystem();
	return 0;
}

uint32_t nn::fssrv::sf::IFileSystemProxy::MountImageDirectory(uint32_t _0, nn::fssrv::sf::IFileSystem*& imageFs) {
	ns_print("Stub implementation for nn::fssrv::sf::IFileSystemProxy::MountImageDirectory");
	//imageFs = new nn::fssrv::sf::IFileSystem, string("Image_") + to_string(_0));
        imageFs = new nn::fssrv::sf::IFileSystem();
	return 0;
}

uint32_t nn::fssrv::sf::IFileSystemProxy::MountContentStorage(uint32_t contentStorageID, nn::fssrv::sf::IFileSystem*& contentFs) {
	ns_print("Stub implementation for nn::fssrv::sf::IFileSystemProxy::MountContentStorage");
	//contentFs = new nn::fssrv::sf::IFileSystem, string("CS_") + to_string(contentStorageID));
        contentFs = new nn::fssrv::sf::IFileSystem();
	return 0;
}

uint32_t nn::fssrv::sf::IFileSystemProxyForLoader::MountCode(nn::ApplicationId TID, int8_t * contentPath, unsigned int contentPath_size, nn::fssrv::sf::IFileSystem*& contentFs) {
	ns_print("Stub implementation for nn::fssrv::sf::IFileSystemProxyForLoader::MountCode");
	//contentFs = new nn::fssrv::sf::IFileSystem, "");
        contentFs = new nn::fssrv::sf::IFileSystem();
	return 0;
}
