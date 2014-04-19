/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file finances.cpp Finances of the user. */

/**
 * @defgroup finances_group Finances data and code.
 */

#include "stdafx.h"
#include "finances.h"
#include "math_func.h"
#include "window.h"

FinancesManager _finances_manager; ///< Storage and retrieval of park financial records.

/** Default constructor. */
Finances::Finances()
{
	this->Reset();
}

/** Destructor. */
Finances::~Finances()
{
}

/** Resets the finance categories to \c 0. */
void Finances::Reset()
{
	this->ride_construct = 0;
	this->ride_running = 0;
	this->land_purchase = 0;
	this->landscaping = 0;
	this->park_tickets = 0;
	this->ride_tickets = 0;
	this->shop_sales = 0;
	this->shop_stock = 0;
	this->food_sales = 0;
	this->food_stock = 0;
	this->staff_wages = 0;
	this->marketing = 0;
	this->research = 0;
	this->loan_interest = 0;
}

/**
 * Total the categories.
 * @return Total amount of transferred money.
 */
Money Finances::GetTotal() const
{
	Money income = this->park_tickets + this->ride_tickets + this->shop_sales + this->food_sales;
	Money expenses = this->ride_construct + this->ride_running + this->land_purchase + this->landscaping +
			this->shop_stock + this->food_stock + this->staff_wages + this->marketing + this->research + this->loan_interest;

	return income + expenses; // Expenses are already negative.
}

/**
 * Load all monies of one month.
 * @param ldr Input stream to load from.
 * @param version Version of the data to load.
 */
void Finances::Load(Loader &ldr, uint32 version)
{
	switch (version) {
		case 0: // Reset to default.
			this->Reset();
			break;

		case 1:
			ride_construct = ldr.GetLongLong();
			ride_running   = ldr.GetLongLong();
			land_purchase  = ldr.GetLongLong();
			landscaping    = ldr.GetLongLong();
			park_tickets   = ldr.GetLongLong();
			ride_tickets   = ldr.GetLongLong();
			shop_sales     = ldr.GetLongLong();
			shop_stock     = ldr.GetLongLong();
			food_sales     = ldr.GetLongLong();
			food_stock     = ldr.GetLongLong();
			staff_wages    = ldr.GetLongLong();
			marketing      = ldr.GetLongLong();
			research       = ldr.GetLongLong();
			loan_interest  = ldr.GetLongLong();
			break;

		default: // Unknown version.
			ldr.SetFailMessage("Unknown version in finances");
			this->Reset();
			break;
	}
}

/**
 * Save all monies of one month.
 * @param svr Output stream to save to.
 */
void Finances::Save(Saver &svr)
{
	svr.PutLongLong((uint64)ride_construct);
	svr.PutLongLong((uint64)ride_running);
	svr.PutLongLong((uint64)land_purchase);
	svr.PutLongLong((uint64)landscaping);
	svr.PutLongLong((uint64)park_tickets);
	svr.PutLongLong((uint64)ride_tickets);
	svr.PutLongLong((uint64)shop_sales);
	svr.PutLongLong((uint64)shop_stock);
	svr.PutLongLong((uint64)food_sales);
	svr.PutLongLong((uint64)food_stock);
	svr.PutLongLong((uint64)staff_wages);
	svr.PutLongLong((uint64)marketing);
	svr.PutLongLong((uint64)research);
	svr.PutLongLong((uint64)loan_interest);
}

/** Default constructor. */
FinancesManager::FinancesManager()
{
	this->Reset();
}

/** Destructor. */
FinancesManager::~FinancesManager()
{
}

/** Reset all finances to initial state. */
void FinancesManager::Reset()
{
	this->num_used = 1;
	this->current = 0;
	this->finances[this->current].Reset();
	this->cash = 0;
}

/**
 * Get the finance object for the current month.
 * @return Current finances object.
 */
const Finances &FinancesManager::GetFinances()
{
	assert(this->current >= 0 && this->current < this->num_used);
	return this->finances[this->current];
}

/** Complete the current month and transition to new finances object. */
void FinancesManager::AdvanceMonth()
{
	this->num_used = std::min(this->num_used + 1, NUM_FINANCE_HISTORY);
	this->current = (this->current + 1) % NUM_FINANCE_HISTORY;
	this->finances[this->current].Reset();
}

/** Transfers cash into #_str_params. */
void FinancesManager::CashToStrParams()
{
	_str_params.SetMoney(1, this->cash);
}

/**
 * Initialize finances with scenario configuration.
 * @param s %Scenario to use for initialization.
 */
void FinancesManager::SetScenario(const Scenario &s)
{
	this->cash = s.inital_money;
}

/**
 * Access method for actually changing amount of money and notifying GUI.
 * @param income How much money to change total by.
 * @note Pass a negative number for a loss of money.
 */
void FinancesManager::DoTransaction(const Money &income)
{
	this->cash += income;
	this->NotifyGui();
}

/** Let the Gui know the financial data has changed. */
void FinancesManager::NotifyGui()
{
	NotifyChange(WC_BOTTOM_TOOLBAR, ALL_WINDOWS_OF_TYPE, CHG_DISPLAY_OLD, 0);
	NotifyChange(WC_FINANCES, ALL_WINDOWS_OF_TYPE, CHG_DISPLAY_OLD, 0);
}

/**
 * Load financial data from the save game.
 * @param ldr Input stream to load from.
 */
void FinancesManager::Load(Loader &ldr)
{
	this->Reset(); // version == 0 already handled.

	uint32 version = ldr.OpenBlock("FINA");
	if (version == 1) {
		this->num_used = ldr.GetByte();
		this->current = ldr.GetByte();
		this->cash = ldr.GetLongLong();
		for (int i = 0; i < this->num_used; i++) this->finances[i].Load(ldr, version);
	} else {
		ldr.SetFailMessage("Unknown block in finances manager.");
	}
	ldr.CloseBlock();
	this->NotifyGui();
}

/**
 * Save financial data to the save game.
 * @param svr Output stream to save to.
 */
void FinancesManager::Save(Saver &svr)
{
	svr.StartBlock("FINA", 1);
	svr.PutByte(this->num_used);
	svr.PutByte(this->current);
	svr.PutLongLong(this->cash);
	for (int i = 0; i < this->num_used; i++) this->finances[i].Save(svr);
	svr.EndBlock();
}
