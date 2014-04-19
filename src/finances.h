/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file finances.h Definition of finances of the park. */

#ifndef FINANCES_H
#define FINANCES_H
#include "money.h"
#include "language.h"
#include "gamelevel.h"

static const int NUM_FINANCE_HISTORY = 4; ///< Number of finance objects to keep for history.

/**
 * Tracking monthly finances.
 * @ingroup finances_group
 */
class Finances {
public:
	Finances();
	~Finances();

	Money ride_construct; ///< Monthly expenditures for ride construction (value is negative).
	Money ride_running;   ///< Monthly expenditures for ride running costs (value is negative).
	Money land_purchase;  ///< Monthly expenditures for land purchase (value is negative).
	Money landscaping;    ///< Monthly expenditures for landscaping (value is negative).
	Money park_tickets;   ///< Monthly earnings for park tickets.
	Money ride_tickets;   ///< Monthly earnings for ride tickets.
	Money shop_sales;     ///< Monthly earnings for shop sales.
	Money shop_stock;     ///< Monthly expenditures for shop stock (value is negative).
	Money food_sales;     ///< Monthly earnings for food sales.
	Money food_stock;     ///< Monthly expenditures for food stock (value is negative).
	Money staff_wages;    ///< Monthly expenditures for staff wages (value is negative).
	Money marketing;      ///< Monthly expenditures for marketing (value is negative).
	Money research;       ///< Monthly expenditures for research (value is negative).
	Money loan_interest;  ///< Monthly expenditures for loan interest (value is negative).

	void Reset();
	Money GetTotal() const;

	void Load(Loader &ldr, uint32 version);
	void Save(Saver &svr);
};

/**
 * A manager of finance objects.
 * @ingroup finances_group
 */
class FinancesManager {
protected:
	Finances finances[NUM_FINANCE_HISTORY]; ///< All finance objects needed for statistics.
	int num_used;                           ///< Number of %Finances objects that have history.
	int current;                            ///< Index for the current month's %Finances object.
	Money cash;                             ///< The user's current cash.

public:
	FinancesManager();
	~FinancesManager();

	const Finances &GetFinances();
	void AdvanceMonth();
	void CashToStrParams();
	void SetScenario(const Scenario &s);

	void DoTransaction(const Money &income);

	void Load(Loader &ldr);
	void Save(Saver &svr);

	/**
	 * Access method for monthly transactions.
	 * @param m how much money to pay.
	 */
	inline void PayRideConstruct(const Money &m)
	{
		this->finances[this->current].ride_construct -= m;
		DoTransaction(-m);
	}

	/**
	 * Access method for monthly transactions.
	 * @param m how much money to pay.
	 */
	inline void PayRideRunning(const Money &m)
	{
		this->finances[this->current].ride_running -= m;
		DoTransaction(-m);
	}

	/**
	 * Access method for monthly transactions.
	 * @param m how much money to pay.
	 */
	inline void PayLandPurchase(const Money &m)
	{
		this->finances[this->current].land_purchase -= m;
		DoTransaction(-m);
	}

	/**
	 * Access method for monthly transactions.
	 * @param m how much money to pay.
	 */
	inline void PayLandscaping(const Money &m)
	{
		this->finances[this->current].landscaping -= m;
		DoTransaction(-m);
	}

	/**
	 * Access method for monthly transactions.
	 * @param m how much money to pay.
	 */
	inline void PayShopStock(const Money &m)
	{
		this->finances[this->current].shop_stock -= m;
		DoTransaction(-m);
	}

	/**
	 * Access method for monthly transactions.
	 * @param m how much money to pay.
	 */
	inline void PayFoodStock(const Money &m)
	{
		this->finances[this->current].food_stock -= m;
		DoTransaction(-m);
	}

	/**
	 * Access method for monthly transactions.
	 * @param m how much money to pay.
	 */
	inline void PayStaffWages(const Money &m)
	{
		this->finances[this->current].staff_wages -= m;
		DoTransaction(-m);
	}

	/**
	 * Access method for monthly transactions.
	 * @param m how much money to pay.
	 */
	inline void PayMarketing(const Money &m)
	{
		this->finances[this->current].marketing -= m;
		DoTransaction(-m);
	}

	/**
	 * Access method for monthly transactions.
	 * @param m how much money to pay.
	 */
	inline void PayResearch(const Money &m)
	{
		this->finances[this->current].research -= m;
		DoTransaction(-m);
	}

	/**
	 * Access method for monthly transactions.
	 * @param m how much money to pay.
	 */
	inline void PayLoanInterest(const Money &m)
	{
		this->finances[this->current].loan_interest -= m;
		DoTransaction(-m);
	}

	/**
	 * Access method for monthly transactions.
	 * @param m how much money to earn.
	 */
	inline void EarnParkTickets(const Money &m)
	{
		this->finances[this->current].park_tickets += m;
		DoTransaction(m);
	}

	/**
	 * Access method for monthly transactions.
	 * @param m how much money to earn.
	 */
	inline void EarnRideTickets(const Money &m)
	{
		this->finances[this->current].ride_tickets += m;
		DoTransaction(m);
	}

	/**
	 * Access method for monthly transactions.
	 * @param m how much money to earn.
	 */
	inline void EarnShopSales(const Money &m)
	{
		this->finances[this->current].shop_sales += m;
		DoTransaction(m);
	}

	/**
	 * Access method for monthly transactions.
	 * @param m how much money to earn.
	 */
	inline void EarnFoodSales(const Money &m)
	{
		this->finances[this->current].food_sales += m;
		DoTransaction(m);
	}

private:
	void Reset();
	void NotifyGui();
};

extern FinancesManager _finances_manager;

#endif
