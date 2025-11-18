library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;
use ieee.numeric_std.all;

entity fsm is
    Port ( CPUwr : in  STD_LOGIC;
           CPUcs : in  STD_LOGIC;
           UCovf : in  STD_LOGIC;
           TVDhit : in  STD_LOGIC;
			  clk: in  STD_LOGIC;
			  TVDdirty: in  STD_LOGIC;
           CPUrs : out  STD_LOGIC;
           SRAMdout : out  STD_LOGIC;
           SRAMdin : out  STD_LOGIC;
           SRAMwrs : out  STD_LOGIC_VECTOR (1 downto 0);
           SDRAMwr : out  STD_LOGIC;
           UCinc : out  STD_LOGIC;
           TVDwrT : out  STD_LOGIC;
           TVDwrV : out  STD_LOGIC;
           TVDwrD : out  STD_LOGIC;
           TVDinV : out  STD_LOGIC;
           TVDinD : out  STD_LOGIC;
			  TVDvalid : in STD_LOGIC
			  );
end fsm;

architecture Behavioral of fsm is
	signal state : integer := 8;
begin
	process(clk)
	begin
		if(clk'Event and clk = '1') then
			if(state = 0) then 
				if(CPUcs = '1') then
					state <= 1;
				end if;
			
			elsif(state = 1) then			
				if(TVDvalid = '0' or (TVDhit = '0' and TVDdirty = '0')) then
					state <= 4;
				elsif(TVDhit = '1' and CPUwr = '1') then
					state <= 2;
				elsif(TVDhit = '1' and CPUwr = '0') then
					state <= 3;
				elsif(TVDhit = '0' and TVDdirty = '1') then
					state <= 5;
				end if;
			
			elsif(state = 2) then
				if(CPUcs = '0') then
					state <= 0;
				end if;
			
			elsif(state = 3) then
				
				if(CPUcs = '0') then
					state <= 0;
				end if;
			
			elsif(state = 4) then
				
				if(UCovf = '1') then
					state <= 6;
				end if;
			
			elsif(state = 5) then
				
				if(UCovf = '1') then
					state <= 7;
				end if;
			
			elsif(state = 6) then
				
				if(CPUwr = '1') then
					state <= 2;
				else
					state <= 3;
				end if;
			
			elsif(state = 7) then
				state <= 4;
				
			elsif(state = 8) then
				state <= 9;
			elsif(state = 9) then
				state <= 0;
			end if;
		end if;
	end process;
	
	process(state)
	begin
		if(state = 0) then 
				CPUrs  <= '1';
				TVDwrT <= '0';
				TVDwrV  <= '0';
				TVDwrD <= '0';
				SDRAMwr <= '0';
				SRAMwrs <= "00";
				UCinc <= '0';
			
		elsif(state = 1) then
				CPUrs <= '0';
			
		elsif(state = 2) then
				SRAMwrs <= "01"; 
				SRAMdin <= '0';
				TVDwrV <= '1';
				TVDwrD <= '1';
				TVDinV <= '1';
				TVDinD <= '1';
	
		elsif(state = 3) then
				SRAMwrs <= "00";
				SRAMdout <= '1';
			
		elsif(state = 4) then
				UCinc <= '1';
				SRAMwrs <= "10";
				SRAMdin <= '1';
				SDRAMwr <= '0';
			
		elsif(state = 5) then
				TVDwrV <= '1';
				TVDinV <= '0';
				UCinc <= '1';
				SRAMwrs <= "00";
				SRAMdout  <= '0'; 
				SDRAMwr <= '1';
			
		elsif(state = 6) then
				TVDwrV <= '1';
				TVDwrD <= '1';
				TVDwrT <= '1';
				TVDinV <= '1';
				TVDinD <= '0';
				UCinc <= '0';
				SRAMwrs <= "00";
				
	   elsif(state = 7) then
				UCinc <= '0';
		
		elsif(state = 8 or state = 9) then
				CPUrs <= '0';
		end if;
	end process;


end Behavioral;
