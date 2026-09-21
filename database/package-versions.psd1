@{
    SchemaVersion = 1
    Versions = @(
        @{
            Version = "v1.0.0"
            Migrations = @(
                "20260622_secondary_password_security.sql"
                "20260622_self_teleport.sql"
                "20260623_async_filter_commands_indexes.sql"
                "20260624_chat_support_tables.sql"
                "20260624_hwidlist_hwid_normalize_optional.sql"
                "20260624_hwidlist_indexes.sql"
                "20260624_webviewer_buttons.sql"
                "20260625_silk_stall_transactions.sql"
                "20260626_lucky_spin.sql"
                "20260627_guide_icon_visibility_settings.sql"
                "20260627_settings_cleanup_and_catalog.sql"
                "20260627_special_offers.sql"
                "20260627_trade_sell_captcha.sql"
                "20260629_clientless_accounts.sql"
                "20260702_drop_logs_icon_setting.sql"
                "20260708_pvp_challenge.sql"
                "20260709_pvp_challenge_freeze.sql"
                "20260710_auto_events.sql"
                "20260710_pvp_challenge_arena_pool.sql"
                "20260711_events_database.sql"
                "20260713_killer_animation_icon_setting.sql"
                "20260713_killer_animations.sql"
                "20260713_new_inventory_design_setting.sql"
                "20260713_offline_stall.sql"
                "20260714_rename_proxy_database_to_kmtguard.sql"
                "20260715_brand_hwid_notice.sql"
                "20260715_shardmanager_gameserver_integration.sql"
                "20260716_fix_green_book_online_time.sql"
                "20260717_kmtguard_drop_battlepass.sql"
                "20260717_kmtguard_drop_empty_kmt_schema.sql"
                "20260717_kmtguard_flatten_procedures_to_dbo.sql"
                "20260717_kmtguard_flatten_tables_to_dbo.sql"
                "20260717_kmtguard_remove_remaining_legacy_names.sql"
                "20260718_log_database_setting.sql"
                "20260720_custom_trade_procedures.sql"
                "20260720_survival_party_auto_event.sql"
                "20260721_region_timer.sql"
                "20260721_survival_solo_auto_event.sql"
                "20260722_bot_protection.sql"
                "20260722_competitive_events.sql"
                "20260722_customer_addlogitem_maxiguard_to_kmtguard.sql"
                "20260722_customer_marssilkscroll_maxiguard_to_kmtguard.sql"
                "20260722_customer_marsstartpackscroll_2_maxiguard_to_kmtguard.sql"
                "20260722_customer_marsstartpackscroll_maxiguard_to_kmtguard.sql"
                "20260722_kmtguard_addsilklive.sql"
                "20260723_security_region_event_suit.sql"
                "20260724_hide_and_seek_event.sql"
                "20260725_menu_like_maxi_setting.sql"
                "20260726_achievement_system_rebuild.sql"
                "20260726_dynamic_ranking_sql_platform.sql"
                "20260726_honor_rank_runtime_refresh.sql"
                "20260726_retire_legacy_fixed_ranking.sql"
                "20260726_vip_tier_configuration.sql"
                "20260727_auto_event_multi_schedule.sql"
                "20260727_item_chest_integrity_and_online_broadcast.sql"
                "20260727_job_control_procedures.sql"
                "20260727_player_style_system_rebuild.sql"
                "20260727_reliable_system_schedule.sql"
            )
            Validation = @(
                "achievement_system_validation.sql"
                "item_chest_integrity_validation.sql"
                "player_style_system_validation.sql"
            )
        }
        @{
            Version = "v1.1.0"
            Migrations = @(
                "20260727_attendance_system_rebuild.sql"
            )
            Validation = @(
                "attendance_system_integration.sql"
                "attendance_system_validation.sql"
            )
        }
        @{
            Version = "v1.2.6"
            Migrations = @(
                "20260728_fix_vip_right_icon_procedures.sql"
            )
            Validation = @()
        }
        @{
            Version = "v1.4.3"
            Migrations = @(
                "20260728_telegram_notifications.sql"
            )
            Validation = @()
        }
        @{
            Version = "v1.6.0"
            Migrations = @(
                "20260729_settings_invalid_values_type_catalog.sql"
            )
            Validation = @()
        }
        @{
            Version = "v1.7.0"
            Migrations = @(
                "20260729_professional_system_scheduler.sql"
            )
            Validation = @()
        }
        @{
            Version = "v1.9.0"
            Migrations = @(
                "20260729_discord_notifications.sql"
            )
            Validation = @(
                "discord_notifications_validation.sql"
            )
        }
        @{
            Version = "v2.6.0"
            Migrations = @(
                "20260730_clientless_party_form.sql"
            )
            Validation = @()
        }
        @{
            Version = "v2.6.6"
            Migrations = @(
                "20260731_unique_history_persistence.sql"
            )
            Validation = @()
        }
        @{
            Version = "v2.6.10"
            Migrations = @(
                "20260731_unique_history_world_identity_fix.sql"
            )
            Validation = @()
        }
        @{
            Version = "v2.6.11"
            Migrations = @(
                "20260731_auto_equip_level_contract.sql"
            )
            Validation = @()
        }
        @{
            Version = "v2.6.16"
            Migrations = @(
                "20260801_disable_durability.sql"
            )
            Validation = @()
        }
        @{
            Version = "v2.7.0"
            Migrations = @(
                "20260801_party_monster_settings.sql"
            )
            Validation = @(
                "party_monster_settings_validation.sql"
            )
        }
        @{
            Version = "v2.7.5"
            Migrations = @(
                "20260803_vip_runtime_recovery_and_history.sql"
            )
            Validation = @(
                "vip_runtime_validation.sql"
            )
        }
        @{
            Version = "v2.7.19"
            Migrations = @(
                "20260805_disable_green_book.sql"
            )
            Validation = @(
                "disable_green_book_validation.sql"
            )
        }
        @{
            Version = "v2.7.20"
            Migrations = @(
                "20260805_vip_system_toggle.sql"
            )
            Validation = @(
                "vip_system_toggle_validation.sql"
            )
        }
        @{
            Version = "v2.11.0"
            Migrations = @(
                "20260808_remove_custom_honor_rank_runtime.sql"
            )
            Validation = @()
        }
        @{
            Version = "v2.13.4"
            Migrations = @(
                "20260808_silk_stall_transaction_integrity.sql"
            )
            Validation = @(
                "silk_stall_transaction_integrity_validation.sql"
            )
        }
        @{
            Version = "v2.13.7"
            Migrations = @(
                "20260809_offline_stall_integrity.sql"
            )
            Validation = @(
                "offline_stall_integrity_validation.sql"
            )
        }
        @{
            Version = "v2.13.8"
            Migrations = @(
                "20260809_gameserver_security_integrity.sql"
            )
            Validation = @(
                "gameserver_security_integrity_validation.sql"
            )
        }
        @{
            Version = "v2.13.11"
            Migrations = @(
                "20260809_silk_stall_transaction_integrity_repair.sql"
            )
            Validation = @(
                "silk_stall_transaction_integrity_repair_validation.sql"
            )
        }
        @{
            Version = "v3.0.0"
            FullBundle = "KMTGuard_FULL_DATABASE_UPDATE_v3.0.0.sql"
            FullBundleBaseVersion = "v2.7.20"
            Migrations = @(
                "20260809_kmtguard_v3_security_runtime.sql"
            )
            Validation = @(
                "kmtguard_v3_security_runtime_validation.sql"
            )
        }
        @{
            Version = "v3.0.4"
            Migrations = @(
                "20260810_pvp_challenge_integrity.sql"
                "20260810_pvp_challenge_procedure_name_repair.sql"
            )
            Validation = @(
                "pvp_challenge_integrity_validation.sql"
            )
        }
        @{
            Version = "v3.0.5"
            Migrations = @(
                "20260810_gameserver_runtime_integrity.sql"
            )
            Validation = @(
                "gameserver_runtime_integrity_validation.sql"
            )
        }
        @{
            Version = "v3.0.6"
            Migrations = @(
                "20260810_gameserver_patch_dashboard.sql"
            )
            Validation = @(
                "gameserver_patch_dashboard_validation.sql"
            )
        }
        @{
            Version = "v3.1.0"
            Migrations = @(
                "20260810_auto_event_recurring_schedule.sql"
            )
            Validation = @(
                "auto_event_recurring_schedule_validation.sql"
            )
        }
        @{
            Version = "v3.1.2"
            Migrations = @(
                "20260810_readpast_rcsi_compatibility.sql"
            )
            Validation = @(
                "readpast_rcsi_compatibility_validation.sql"
            )
        }
        @{
            Version = "v3.1.3"
            Migrations = @(
                "20260810_readpast_session_isolation.sql"
            )
            Validation = @(
                "readpast_session_isolation_validation.sql"
            )
        }
        @{
            Version = "v3.2.0"
            Migrations = @(
                "20260810_clientless_city_groups.sql"
            )
            Validation = @(
                "clientless_city_groups_validation.sql"
            )
        }
        @{
            Version = "v3.3.0"
            Migrations = @(
                "20260812_clientless_hunting.sql"
            )
            Validation = @(
                "clientless_hunting_validation.sql"
            )
        }
        @{
            Version = "v3.7.0"
            Migrations = @(
                "20260814_clientless_creation_options.sql"
            )
            Validation = @(
                "clientless_creation_options_validation.sql"
            )
        }
        @{
            Version = "v3.8.0"
            Migrations = @(
                "20260814_clientless_party_modes.sql"
            )
            Validation = @(
                "clientless_party_modes_validation.sql"
            )
        }
        @{
            Version = "v3.9.0"
            Migrations = @(
                "20260814_clientless_independent_pets.sql"
            )
            Validation = @(
                "clientless_independent_pets_validation.sql"
            )
        }
        @{
            Version = "v3.9.5"
            Migrations = @(
                "20260814_refskill_item_orphan_cleanup.sql"
            )
            Validation = @(
                "refskill_item_orphan_cleanup_validation.sql"
            )
        }
        @{
            Version = "v6.0.0"
            Migrations = @(
                "20260817_drop_monster_window.sql"
                "20260824_hook_spawn_complete.sql"
                "20260825_item_region_restrictions.sql"
                "20260826_region_admission_hardening.sql"
            )
            Validation = @(
                "region_admission_hardening_validation.sql"
            )
        }
        @{
            Version = "v6.1.0"
            Migrations = @(
                "20260829_action_wnd_commands.sql"
            )
            Validation = @()
        }
        @{
            Version = "v6.2.0"
            Migrations = @(
                "20260829_gm_runtime_controls.sql"
            )
            Validation = @(
                "gm_runtime_controls_validation.sql"
            )
        }
        @{
            Version = "v6.2.1"
            Migrations = @(
                "20260829_action_wnd_whitelist.sql"
            )
            Validation = @()
        }
        @{
            Version = "v6.2.2"
            Migrations = @(
                "20260829_remove_runtime_event_bridge.sql"
            )
            Validation = @()
        }
        @{
            Version = "v6.2.7"
            Migrations = @(
                "20260830_menu_casy_setting.sql"
            )
            Validation = @(
                "menu_casy_setting_validation.sql"
            )
        }
        @{
            Version = "v6.3.9"
            Migrations = @(
                "20260909_disable_original_trade_gold.sql"
            )
            Validation = @(
                "disable_original_trade_gold_validation.sql"
            )
        }
    )
}
