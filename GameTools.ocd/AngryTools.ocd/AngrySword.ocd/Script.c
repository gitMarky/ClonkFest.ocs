/*--    DeadlyTool    --*/


#include AngryTool


func Initialize()
{
	inherited(...);
	SetGraphics(nil, Sword);
	PlayAnimation("Base", 5, Anim_Const(0), Anim_Const(1000));
	AddEffect("HitCheck", this, 1,2, nil,nil);
}

func ContactBottom()
{
	inherited(...);
	var particle = Particles_Spark();
	particle.R = 0xff;
	particle.G = 0xff;
	particle.B = PV_Random(150,200);
	CreateParticle("MagicSpark", 0,ReturnHeight()/2-1, PV_Random(-10,10), PV_Random(-10,10), PV_Random(10,50), particle, 15);
	Sound("Hits::Materials::Metal::LightMetalHit?", false, 20);
}


public func HitObject(object obj)
{	
	if(obj->GetID()==Clonk)
	{
	obj->~OnProjectileHit(this);
	WeaponTumble(obj, this->TumbleStrength());
	obj->SetYDir(-50);
	obj->Hurt();
	RemoveEffect("HitCheck", this);
	AddEffect("Cooldown", this, 20, 10);
	}
}

public func TumbleStrength() { return 100; }

func FxCooldownTimer(target, effect, effect_time)
{
	if(effect_time > 3) 
	{
		AddEffect("HitCheck", this, 1,2, nil,nil);
		return -1;
	}
}

public func OnStrike(object obj)
{
	if(obj->GetAlive())
		Sound("Hits::ProjectileHitLiving?", false, 50);
}

func ReturnDamage()
{
	return(30);
}
