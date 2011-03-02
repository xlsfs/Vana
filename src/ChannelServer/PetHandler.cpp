/*
Copyright (C) 2008-2011 Vana Development Team

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2
of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include "PetHandler.h"
#include "Inventory.h"
#include "InventoryPacket.h"
#include "ItemDataProvider.h"
#include "MovementHandler.h"
#include "PacketReader.h"
#include "Pet.h"
#include "PetsPacket.h"
#include "Player.h"
#include "Randomizer.h"

void PetHandler::handleMovement(Player *player, PacketReader &packet) {
	int32_t petid = packet.get<int32_t>();
	Pet *pet = player->getPets()->getPet(petid);
	packet.skipBytes(8);
	MovementHandler::parseMovement(pet, packet);
	packet.reset(10);
	PetsPacket::showMovement(player, pet, packet.getBuffer(), packet.getBufferLength() - 9);
}

void PetHandler::handleChat(Player *player, PacketReader &packet) {
	int32_t petid = packet.get<int32_t>();
	packet.skipBytes(5);
	int8_t act = packet.get<int8_t>();
	string message = packet.getString();
	PetsPacket::showChat(player, player->getPets()->getPet(petid), message, act);
}

void PetHandler::handleSummon(Player *player, PacketReader &packet) {
	packet.skipBytes(4);
	int16_t slot = packet.get<int16_t>();
	bool master = packet.get<int8_t>() == 1; // Might possibly fit under getBool criteria
	bool multipet = player->getSkills()->getSkillLevel(Jobs::Beginner::FollowTheLead) > 0;
	Pet *pet = player->getPets()->getPet(player->getInventory()->getItem(Inventories::CashInventory, slot)->petid);

	if (pet->isSummoned()) { // Removing a pet
		player->getPets()->setSummoned(pet->getIndex(), 0);
		if (pet->getIndex() == 0) {
			Timer::Id id(Timer::Types::PetTimer, pet->getIndex(), 0);
			player->getTimers()->removeTimer(id);
		}
		if (multipet) {
			for (int8_t i = pet->getIndex(); i < Inventories::MaxPetCount; i++) { // Shift around pets if using multipet
				if (Pet *move = player->getPets()->getSummoned(i)) {
					move->setIndex(i - 1);
					player->getPets()->setSummoned(move->getIndex(), move->getId());
					player->getPets()->setSummoned(i, 0);
					if (move->getIndex() == 0)
						move->startTimer();
				}
			}
		}
		int8_t index = pet->getIndex();
		pet->setIndex(-1);
		PetsPacket::petSummoned(player, pet, false, false, index);
	}
	else { // Summoning a Pet
		pet->setPos(player->getPos());
		if (!multipet || master) {
			pet->setIndex(0);
			if (multipet) {
				for (int8_t i = Inventories::MaxPetCount - 1; i > 0; i--) {
					if (player->getPets()->getSummoned(i - 1) && !player->getPets()->getSummoned(i)) {
						Pet *move = player->getPets()->getSummoned(i - 1);
						player->getPets()->setSummoned(i, move->getId());
						player->getPets()->setSummoned(i - 1, 0);
						move->setIndex(i);
					}
				}
				PetsPacket::petSummoned(player, pet);
			}
			else if (Pet *kicked = player->getPets()->getSummoned(0)) {
				kicked->setIndex(-1);
				Timer::Id id(Timer::Types::PetTimer, kicked->getIndex(), 0);
				player->getTimers()->removeTimer(id);
				PetsPacket::petSummoned(player, pet, true);
			}
			else
				PetsPacket::petSummoned(player, pet);

			player->getPets()->setSummoned(0, pet->getId());
			pet->startTimer();
		}
		else {
			for (int8_t i = 0; i < Inventories::MaxPetCount; i++) {
				if (!player->getPets()->getSummoned(i)) {
					player->getPets()->setSummoned(i, pet->getId());
					pet->setIndex(i);
					PetsPacket::petSummoned(player, pet);
					pet->startTimer();
					break;
				}
			}
		}
	}
	PetsPacket::blankUpdate(player);
}

void PetHandler::handleFeed(Player *player, PacketReader &packet) {
	packet.skipBytes(4);
	int16_t slot = packet.get<int16_t>();
	int32_t item = packet.get<int32_t>();
	if (Pet *pet = player->getPets()->getSummoned(0)) {
		Inventory::takeItem(player, item, 1);

		bool success = (pet->getFullness() < Stats::MaxFullness);
		PetsPacket::showAnimation(player, pet, 1, success);
		if (success) {
			pet->modifyFullness(Stats::PetFeedFullness, false);
			if (Randomizer::Instance()->randInt(99) < 60) // 60% chance for feed to add closeness
				pet->addCloseness(1);
		}
	}
	else {
		InventoryPacket::blankUpdate(player);
	}
}

void PetHandler::handleCommand(Player *player, PacketReader &packet) {
	int32_t petid = packet.get<int32_t>();
	packet.skipBytes(5);
	int8_t act = packet.get<int8_t>();
	Pet *pet = player->getPets()->getPet(petid);
	PetInteractInfo *action = ItemDataProvider::Instance()->getInteraction(pet->getItemId(), act);
	if (action == nullptr)
		return;
	bool success = (Randomizer::Instance()->randInt(100) < action->prob);
	if (success) {
		pet->addCloseness(action->increase);
	}
	PetsPacket::showAnimation(player, pet, act, success);
}

void PetHandler::changeName(Player *player, const string &name) {
	if (Pet *pet = player->getPets()->getSummoned(0)) {
		pet->setName(name);
	}
}

void PetHandler::showPets(Player *player) {
	for (int8_t i = 0; i < Inventories::MaxPetCount; i++) {
		if (Pet *pet = player->getPets()->getSummoned(i)) {
			pet->setPos(player->getPos());
			PetsPacket::petSummoned(player, pet, false, true);
		}
	}
	PetsPacket::updateSummonedPets(player);
}