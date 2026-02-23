# ARPG Gameplay Test – GAS Leap Slam

## Overview
A focused ARPG combat prototype built in Unreal Engine 5.5 on the C++ Top-Down template.
Demonstrates correct Gameplay Ability System (GAS) usage through a Leap Slam ability
inspired by Path of Exile. The goal is clean architecture and extensible GAS foundations,
not content volume.

## Tech Stack
- **Engine:** Unreal Engine 5.5
- **Template:** Top-Down C++
- **Input:** Enhanced Input System
- **Combat:** Gameplay Ability System (GAS)

## Architecture
All gameplay flows through GAS. No direct stat modification anywhere in the codebase.
AbilitySystemComponent is attached to both the player and enemies. All combat interactions —
damage, costs, cooldowns — are handled exclusively through GameplayEffects.

## Attribute System
Custom `UPlayerAttributeSet` with:
- Health / MaxHealth
- Mana / MaxMana
- AttackSpeed

Attributes use `FGameplayAttributeData` and support modifiers. Death triggers when Health
reaches zero. Attribute changes drive UI updates via delegate binding — no tick polling.

## Leap Slam Ability
Activated on right mouse button click. The character launches toward the cursor position
using `LaunchCharacter` with a flat XY direction and separate Z force for arc control.

**Flow:**
1. Right mouse click fires `TryActivateAbilityByClass`
2. `CommitAbility` validates and deducts Mana cost
3. Cursor hit is resolved — enemy target or ground position
4. Character faces and launches toward target
5. After `DamageDelay`, sphere overlap applies damage via instant GameplayEffect
6. Movement is restored and ability ends

**Key details:**
- Launch direction flattened to XY, Z applied separately for clean arc
- Damage uses `SetByCaller` magnitude for flexibility
- Mana deducted through GameplayEffect, not directly
- Ability fails cleanly if Mana is insufficient

## Mana Restore
When Mana decreases, a 2.5s delayed timer applies a restore GameplayEffect.
Active mana effects are cleared on restore or when Mana reaches max.
All handled via attribute change delegate — no manual checks in Tick.

## Enemy Setup
Minimal enemy with ASC and AttributeSet. Takes damage through GameplayEffects.
No AI by design — focus is on the combat system, not behavior.

## UI
Health and Mana bars update via GAS attribute change delegates.
Enemy floating health bar reflects current health percentage.
No tick-based UI updates.

## What This Demonstrates
- Correct GAS setup from scratch in C++
- Clean ability flow with commit, cost, cooldown, and damage
- Event-driven UI without tick
- Modular structure ready for new abilities, buffs, damage types, and weapon systems