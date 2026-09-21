USE [KMTGuard];
GO

IF OBJECT_ID(N'[dbo].[_LotteryGold_Register_JT]', N'SN') IS NOT NULL
    DROP SYNONYM [dbo].[_LotteryGold_Register_JT];
GO

IF OBJECT_ID(N'[dbo].[_LotteryGold_Register_KMTGuard]', N'SN') IS NULL
    EXEC(N'CREATE SYNONYM [dbo].[_LotteryGold_Register_KMTGuard] FOR [KMTGuard].[dbo].[Event_RegisterGoldLottery]');
GO

IF OBJECT_ID(N'[dbo].[PK__RefVFilterAvatarall]', N'PK') IS NOT NULL
    EXEC sp_rename N'[dbo].[PK__RefVFilterAvatarall]', N'PK_Mall_Avatars', N'OBJECT';
GO

IF OBJECT_ID(N'[dbo].[PK__RefVFilterItemMall]', N'PK') IS NOT NULL
    EXEC sp_rename N'[dbo].[PK__RefVFilterItemMall]', N'PK_Mall_Items', N'OBJECT';
GO

IF OBJECT_ID(N'[dbo].[PK__RefVFilterItemMallCategories]', N'PK') IS NOT NULL
    EXEC sp_rename N'[dbo].[PK__RefVFilterItemMallCategories]', N'PK_Mall_Categories', N'OBJECT';
GO
