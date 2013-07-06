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
 * @return Total amount of transfered money.
 */
Money Finances::GetTotal() const
{
	Money income = this->park_tickets + this->ride_tickets + this->shop_sales + this->food_sales;
	Money expenses = this->ride_construct + this->ride_running + this->land_purchase + this->landscaping +
			this->shop_stock + this->food_stock + this->staff_wages + this->marketing + this->research + this->loan_interest;

	return income + expenses; // Expenses are already negative.
}

/** Default constructor. */
FinancesManager::FinancesManager()
{
	this->num_used = 1;
	this->current = 0;
}

/** Destructor. */
FinancesManager::~FinancesManager()
{
}

/**
 * Get the finance object for the current month.
 */
const Finances &FinancesManager::GetFinances()
{
	assert(this->current >= 0 && this->current < this->num_used);
	return this->finances[this->current];
}

/**
 * Complete the current month and transition to new finances object.
 */
void FinancesManager::AdvanceMonth()
{
	this->num_used = min(this->num_used + 1, NUM_FINANCE_HISTORY);
	this->current = (this->current + 1) % NUM_FINANCE_HISTORY;
	this->finances[this->current].Reset();
}

/**
 * Transfers cash into #_str_params.
 */
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
	NotifyChange(WC_BOTTOM_TOOLBAR, ALL_WINDOWS_OF_TYPE, CHG_DISPLAY_OLD, 0);
	NotifyChange(WC_FINANCES, ALL_WINDOWS_OF_TYPE, CHG_DISPLAY_OLD, 0);
}
